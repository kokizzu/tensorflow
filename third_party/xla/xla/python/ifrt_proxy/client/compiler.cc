// Copyright 2023 The OpenXLA Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "xla/python/ifrt_proxy/client/compiler.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "llvm/Support/Casting.h"
#include "xla/debug_options_flags.h"
#include "xla/pjrt/host_callback.h"
#include "xla/python/ifrt/client.h"
#include "xla/python/ifrt/compiler.h"
#include "xla/python/ifrt/device.h"
#include "xla/python/ifrt/device_list.h"
#include "xla/python/ifrt/executable.h"
#include "xla/python/ifrt/future.h"
#include "xla/python/ifrt/host_callback.h"
#include "xla/python/ifrt/program.h"
#include "xla/python/ifrt/serdes.h"
#include "xla/python/ifrt/topology.h"
#include "xla/python/ifrt_proxy/client/executable.h"
#include "xla/python/ifrt_proxy/client/rpc_helper.h"
#include "xla/python/ifrt_proxy/common/ifrt_service.pb.h"
#include "xla/python/ifrt_proxy/common/versions.h"
#include "xla/python/ifrt_proxy/server/host_callback.h"
#include "xla/python/pjrt_ifrt/pjrt_host_callback.h"
#include "xla/python/pjrt_ifrt/xla_compiler.h"
#include "xla/tsl/concurrency/ref_count.h"
#include "xla/tsl/platform/errors.h"
#include "xla/tsl/platform/status_to_from_proto.h"
#include "xla/tsl/platform/statusor.h"
#include "tsl/profiler/lib/traceme.h"

namespace xla {
namespace ifrt {
namespace proxy {

Compiler::Compiler(xla::ifrt::Client* client,
                   std::shared_ptr<RpcHelper> rpc_helper)
    : client_(client), rpc_helper_(std::move(rpc_helper)) {}

absl::StatusOr<xla::ifrt::LoadedExecutableRef> Compiler::CompileAndLoad(
    std::unique_ptr<Program> program,
    std::unique_ptr<xla::ifrt::CompileOptions> options) {
  auto request = std::make_unique<CompileRequest>();
  {
    tsl::profiler::TraceMe traceme("IfrtProxyProgramSerialize");
    auto serialize_options = std::make_unique<xla::ifrt::SerializeOptions>(
        rpc_helper_->ifrt_serdes_version());
    TF_ASSIGN_OR_RETURN(*request->mutable_program(),
                        Serialize(*program, std::move(serialize_options)));
  }
  tsl::profiler::TraceMe traceme_ifrt_entrypoint(
      [prog_size = request->program().data().size()]() {
        return tsl::profiler::TraceMeEncode(
            "IfrtProxyEntrypointCompilerCompile", {{"prog_size", prog_size}});
      });

  // Extract host callbacks from the XLA compile options. `XlaCompileOptions`'s
  // SerDes fails when it contains host callbacks, so the following
  // implementation handles host callback serialization out of band until we can
  // natively support IFRT host callback on IFRT proxy.
  std::vector<tsl::RCReference<xla::ifrt::LoadedHostCallback>>
      loaded_host_callbacks;
  if (auto* xla_options =
          llvm::dyn_cast<xla::ifrt::XlaCompileOptions>(options.get())) {
    for (const auto& loaded_host_callback :
         xla_options->loaded_host_callbacks) {
      auto* pjrt_host_callback =
          llvm::dyn_cast<xla::ifrt::PjRtHostSendAndRecvLoadedHostCallback>(
              loaded_host_callback.get());
      if (pjrt_host_callback == nullptr) {
        return absl::UnimplementedError("Unsupported host callback type");
      }

      const xla::HostCallback& xla_host_callback =
          pjrt_host_callback->host_callback();

      // The proxy server runs `RemoteLoadedHostCallback` that delegates actual
      // host callback execution to the proxy client.
      auto remote_loaded_host_callback = tsl::MakeRef<RemoteLoadedHostCallback>(
          client_, xla_host_callback.operands, xla_host_callback.results,
          /*queue=*/nullptr);
      TF_ASSIGN_OR_RETURN(*request->add_host_callbacks(),
                          remote_loaded_host_callback->Serialize());
    }

    loaded_host_callbacks.swap(xla_options->loaded_host_callbacks);

#if defined(PLATFORM_GOOGLE)
    // Capture XLA flags.
    // This is disabled for OSS because, if not, it creates a difference in
    // behavior between OSS XLA_FLAGS and XLA TPU flags. XLA_FLAGS would be
    // captured here and propagated to the server, but XLA TPU flags would not.
    //
    // With the current implementation both XLA_FLAGS and XLA TPU flags should
    // be set at the proxy server in OSS/Cloud. For google internal usecases,
    // both should be set at the proxy client.
    auto& build_options = xla_options->compile_options.executable_build_options;
    *build_options.mutable_debug_options() = xla::GetDebugOptionsFromFlags();
    TF_RETURN_IF_ERROR(
        build_options.mutable_comp_envs()->InitializeAllKnownEnvs());
#endif
  }

  auto serialize_options = std::make_unique<xla::ifrt::SerializeOptions>(
      rpc_helper_->ifrt_serdes_version());
  TF_ASSIGN_OR_RETURN(*request->mutable_compile_options(),
                      Serialize(*options, std::move(serialize_options)));

  // TODO(b/266635130): Avoid blocking the caller.
  TF_ASSIGN_OR_RETURN(std::shared_ptr<CompileResponse> response,
                      rpc_helper_->Compile(std::move(request)).Await());

  std::vector<xla::ifrt::Device*> addressable_devices;
  addressable_devices.reserve(response->addressable_device_ids_size());
  for (const int32_t device_id : response->addressable_device_ids()) {
    TF_ASSIGN_OR_RETURN(xla::ifrt::Device* const device,
                        client_->LookupDevice(DeviceId(device_id)));
    addressable_devices.push_back(device);
  }

  absl::StatusOr<std::optional<std::string>> fingerprint;
  switch (response->fingerprint_case()) {
    case CompileResponse::kFingerprintValue:
      fingerprint = response->fingerprint_value();
      break;
    case CompileResponse::kFingerprintError:
      fingerprint = tsl::StatusFromProto(response->fingerprint_error());
      break;
    default:
      fingerprint = std::nullopt;
      break;
  }

  Future<> ready_future =
      rpc_helper_->CheckFuture(response->ready_future_handle());

  std::vector<uint64_t> loaded_host_callback_handles(
      response->loaded_host_callback_handles().begin(),
      response->loaded_host_callback_handles().end());

  std::vector<xla::ifrt::Device*> devices;
  if (rpc_helper_->protocol_version() < protocol_version::kExecutableDevices) {
    devices.reserve(response->addressable_device_ids_size());
    for (const int32_t device_id : response->addressable_device_ids()) {
      TF_ASSIGN_OR_RETURN(xla::ifrt::Device* const device,
                          client_->LookupDevice(DeviceId(device_id)));
      devices.push_back(device);
    }
  } else {
    devices.reserve(response->device_ids_size());
    for (const int32_t device_id : response->device_ids()) {
      TF_ASSIGN_OR_RETURN(xla::ifrt::Device* const device,
                          client_->LookupDevice(DeviceId(device_id)));
      devices.push_back(device);
    }
  }
  TF_ASSIGN_OR_RETURN(DeviceListRef device_list,
                      client_->MakeDeviceList(devices));

  return std::make_unique<LoadedExecutable>(
      client_, rpc_helper_, response->loaded_executable_handle(),
      response->name(), response->num_devices(), device_list,
      std::move(addressable_devices), std::move(fingerprint),
      std::move(ready_future), std::move(loaded_host_callbacks),
      std::move(loaded_host_callback_handles));
}

absl::StatusOr<xla::ifrt::ExecutableRef> Compiler::Compile(
    std::unique_ptr<Program> program, const Topology& topology,
    std::unique_ptr<CompileOptions> options) {
  return absl::UnimplementedError(
      "IFRT service compiler does not support `Compile` with a topology");
}

absl::StatusOr<xla::ifrt::LoadedExecutableRef>
Compiler::DeserializeLoadedExecutable(
    absl::string_view serialized,
    std::unique_ptr<xla::ifrt::DeserializeExecutableOptions> options) {
  return absl::UnimplementedError(
      "IFRT service compiler does not support `DeserializeLoadedExecutable` "
      "since the underlying serialization format is not stable");
}

char Compiler::ID = 0;

}  // namespace proxy
}  // namespace ifrt
}  // namespace xla
