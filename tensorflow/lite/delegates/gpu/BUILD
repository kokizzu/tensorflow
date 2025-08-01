load("@bazel_skylib//lib:selects.bzl", "selects")
load("@build_bazel_rules_apple//apple:ios.bzl", "ios_static_framework")
load("@build_bazel_rules_apple//apple:macos.bzl", "macos_dylib")
load(
    "//tensorflow/core/platform:build_config_root.bzl",
    "tf_gpu_tests_tags",
)
load("//tensorflow/lite:build_def.bzl", "CXX17_BAZEL_ONLY_COPTS", "tflite_pagesize_linkopts")
load("//tensorflow/lite:special_rules.bzl", "tflite_extra_gles_deps", "tflite_portable_test_suite")
load("//tensorflow/lite/delegates/gpu:build_defs.bzl", "gpu_delegate_linkopts")

package(
    # copybara:uncomment default_applicable_licenses = ["//tensorflow:license"],
    default_visibility = ["//visibility:public"],
    licenses = ["notice"],
)

exports_files([
    "delegate.h",
    "delegate_options.h",
    "metal_delegate.h",
])

_DELEGATE_NO_GL_DEPS = select({
    "//tensorflow:android": [
        ":async_buffers",
        "//tensorflow/lite/core/async/c:task",
        "//tensorflow/lite/core/async/interop/c:attribute_map",
        "//tensorflow/lite/core/async/interop/c:constants",
        "//tensorflow/lite/delegates/gpu/gl:android_sync",
        "//tensorflow/lite/delegates/gpu/gl:egl_environment",
        "//tensorflow/lite/delegates/utils",
        "//tensorflow/lite/delegates/utils:async_type_helpers",
        "//tensorflow/lite/delegates/utils:ret_macros",
        "//tensorflow/lite/delegates/utils:sync_fence",
    ],
    "//conditions:default": [],
}) + [
    ":android_hardware_buffer",
    ":api",
    ":delegate_options",
    ":tflite_profile",
    #"//third_party/GL:EGL_headers",
    #"//third_party/GL:GLES3_headers",
    # go/keep-sorted start
    "@com_google_absl//absl/container:flat_hash_map",
    "@com_google_absl//absl/container:flat_hash_set",
    "@com_google_absl//absl/memory",
    "@com_google_absl//absl/strings",
    "@com_google_absl//absl/types:span",
    "//tensorflow/lite/async:backend_async_kernel_interface",
    "//tensorflow/lite/core/async/interop/c:types",
    "//tensorflow/lite/core/c:common",
    "//tensorflow/lite/delegates/gpu/cl:util",
    "//tensorflow/lite/delegates/gpu/common:data_type",
    "//tensorflow/lite/delegates/gpu/common:model",
    "//tensorflow/lite/delegates/gpu/common:model_builder",
    "//tensorflow/lite/delegates/gpu/common:model_builder_helper",
    "//tensorflow/lite/delegates/gpu/common:quantization_util",
    "//tensorflow/lite/delegates/gpu/common:status",
    "//tensorflow/lite/delegates:serialization",
    "//tensorflow/lite/kernels:kernel_util",
    "//tensorflow/lite/profiling/telemetry",
    "//tensorflow/lite/profiling/telemetry/c:telemetry_setting",
    "//tensorflow/lite/profiling/telemetry/c:telemetry_setting_internal",
    "//tensorflow/lite/profiling/telemetry:telemetry_status",
    "//tensorflow/lite:kernel_api",
    "//tensorflow/lite:minimal_logging",
    # go/keep-sorted end
]

config_setting(
    name = "tflite_gpu_binary_release",
    values = {"copt": "-DTFLITE_GPU_BINARY_RELEASE"},
)

config_setting(
    name = "tflite_gpu_extra_gles_deps",
    constraint_values = [
        "//third_party/bazel_platforms/cpu:x86_64",
        "//third_party/bazel_platforms/os:linux",
    ],
    values = {
        "copt": "-DTFLITE_GPU_EXTRA_GLES_DEPS",
    },
)

cc_library(
    name = "gl_delegate",
    srcs = ["gl_delegate.cc"],
    hdrs = ["gl_delegate.h"],
    linkopts = gpu_delegate_linkopts(),
    deps = [
        "//tensorflow/lite:kernel_api",
        "//tensorflow/lite:minimal_logging",
        "//tensorflow/lite/core/c:common",
        "//tensorflow/lite/delegates/gpu/common:convert",
        "//tensorflow/lite/delegates/gpu/common:gpu_info",
        "//tensorflow/lite/delegates/gpu/common:model",
        "//tensorflow/lite/delegates/gpu/common:model_builder",
        "//tensorflow/lite/delegates/gpu/common:model_transformer",
        "//tensorflow/lite/delegates/gpu/common:shape",
        "//tensorflow/lite/delegates/gpu/common:status",
        "//tensorflow/lite/delegates/gpu/common:tensor",
        "//tensorflow/lite/delegates/gpu/common/transformations:model_transformations",
        "//tensorflow/lite/delegates/gpu/gl:api",
        "//tensorflow/lite/delegates/gpu/gl:command_queue",
        "//tensorflow/lite/delegates/gpu/gl:compiler",
        "//tensorflow/lite/delegates/gpu/gl:compiler_options",
        "//tensorflow/lite/delegates/gpu/gl:egl_environment",
        "//tensorflow/lite/delegates/gpu/gl:gl_buffer",
        "//tensorflow/lite/delegates/gpu/gl:gl_call",
        "//tensorflow/lite/delegates/gpu/gl:object",
        "//tensorflow/lite/delegates/gpu/gl:object_manager",
        "//tensorflow/lite/delegates/gpu/gl:request_gpu_info",
        "//tensorflow/lite/delegates/gpu/gl:runtime_options",
        "//tensorflow/lite/delegates/gpu/gl/converters:bhwc_to_phwc4",
        "//tensorflow/lite/delegates/gpu/gl/converters:phwc4_to_bhwc",
        "//tensorflow/lite/delegates/gpu/gl/kernels:registry",
        "//tensorflow/lite/delegates/gpu/gl/workgroups:best_effort_calculator",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/types:span",
    ] + select({
        "//conditions:default": [
            "//tensorflow/lite/delegates/gpu/gl:common_cc_fbs",
            "//tensorflow/lite/delegates/gpu/gl:metadata_cc_fbs",
            "//tensorflow/lite/delegates/gpu/gl:workgroups_cc_fbs",
            "//tensorflow/lite/schema:schema_fbs",
            "@flatbuffers",
        ],
        ":tflite_gpu_binary_release": [],
    }) + tflite_extra_gles_deps(),
)

objc_library(
    name = "metal_delegate",
    srcs = ["metal_delegate.mm"],
    hdrs = ["metal_delegate.h"],
    copts = CXX17_BAZEL_ONLY_COPTS,
    module_name = "TensorFlowLiteCMetal",
    sdk_frameworks = ["Metal"],
    deps = [
        "//tensorflow/lite:kernel_api",
        "//tensorflow/lite:minimal_logging",
        "//tensorflow/lite/core/c:common",
        "//tensorflow/lite/delegates/gpu/common:convert",
        "//tensorflow/lite/delegates/gpu/common:gpu_info",
        "//tensorflow/lite/delegates/gpu/common:model",
        "//tensorflow/lite/delegates/gpu/common:model_builder",
        "//tensorflow/lite/delegates/gpu/common:model_transformer",
        "//tensorflow/lite/delegates/gpu/common:precision",
        "//tensorflow/lite/delegates/gpu/common:quantization_util",
        "//tensorflow/lite/delegates/gpu/common:shape",
        "//tensorflow/lite/delegates/gpu/common:status",
        "//tensorflow/lite/delegates/gpu/common:types",
        "//tensorflow/lite/delegates/gpu/metal:buffer_convert",
        "//tensorflow/lite/delegates/gpu/metal:common",
        "//tensorflow/lite/delegates/gpu/metal:inference_context",
        "//tensorflow/lite/delegates/gpu/metal:metal_spatial_tensor",
        "//tensorflow/lite/kernels:kernel_util",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/types:span",
    ],
)

objc_library(
    name = "metal_delegate_internal",
    hdrs = ["metal_delegate_internal.h"],
    copts = CXX17_BAZEL_ONLY_COPTS,
    sdk_frameworks = ["Metal"],
    deps = ["//tensorflow/lite/delegates/gpu:metal_delegate"],
)

# build -c opt --config android_arm64 --copt -Os --copt -DTFLITE_GPU_BINARY_RELEASE --linkopt -s --strip always :libtensorflowlite_gpu_gl.so
cc_binary(
    name = "libtensorflowlite_gpu_gl.so",
    linkopts = [
        "-Wl,-soname=libtensorflowlite_gpu_gl.so",
    ] + gpu_delegate_linkopts() + select({
        "//tensorflow:windows": [],
        "//conditions:default": [
            "-fvisibility=hidden",
        ],
    }) + tflite_pagesize_linkopts(),
    linkshared = 1,
    linkstatic = 1,
    tags = [
        "nobuilder",
        "notap",
    ],
    deps = [":gl_delegate"],
)

# build -c opt --config android_arm64 --copt -Os --copt -DTFLITE_GPU_BINARY_RELEASE --linkopt -s --strip always :libtensorflowlite_gpu_delegate.so
cc_binary(
    name = "libtensorflowlite_gpu_delegate.so",
    linkopts = [
        "-Wl,-soname=libtensorflowlite_gpu_delegate.so",
    ] + gpu_delegate_linkopts() + select({
        "//tensorflow:windows": [],
        "//conditions:default": [
            "-fvisibility=hidden",
        ],
    }),
    linkshared = 1,
    linkstatic = 1,
    tags = [
        "nobuilder",
        "notap",
    ],
    deps = [":delegate"],
)

# bazel build -c opt --cpu ios_arm64 --copt -Os --copt -DTFLITE_GPU_BINARY_RELEASE --copt -fvisibility=hidden --linkopt -s --strip always --cxxopt=-std=c++14 :libtensorflowlite_gpu_metal --apple_platform_type=ios
ios_static_framework(
    name = "tensorflow_lite_gpu_framework",
    hdrs = [
        "metal_delegate.h",
        "metal_delegate_internal.h",
    ],
    minimum_os_version = "12.0",
    deps = [":metal_delegate"],
)

# Note: Support for MacOS is best-effort at the moment.
# bazel build -c opt --copt -Os --copt -DTFLITE_GPU_BINARY_RELEASE --copt -fvisibility=hidden --linkopt -s --strip always --cxxopt=-std=c++14 :tensorflow_lite_gpu_dylib --apple_platform_type=macos
macos_dylib(
    name = "tensorflow_lite_gpu_dylib",
    linkopts = [
        "-all_load",
        "-dead_strip",
    ],
    minimum_os_version = "12.0",
    tags = [
        "manual",
        "nobuilder",
        "notap",
    ],
    deps = [
        ":metal_delegate",
        ":metal_delegate_internal",
    ],
)

cc_library(
    name = "api",
    srcs = ["api.cc"],
    hdrs = ["api.h"],
    deps = [
        "//tensorflow/lite/delegates/gpu/common:data_type",
        "//tensorflow/lite/delegates/gpu/common:status",
        "//tensorflow/lite/delegates/gpu/common:util",
        "//tensorflow/lite/delegates/gpu/gl:portable",
        "@com_google_absl//absl/types:span",
        "@com_google_absl//absl/types:variant",
        "@opencl_headers",
        "@vulkan_headers//:vulkan_headers_no_prototypes",
    ],
)

cc_library(
    name = "spi",
    hdrs = ["spi.h"],
    deps = [
        ":api",
        "//tensorflow/lite/delegates/gpu/common:access_type",
        "//tensorflow/lite/delegates/gpu/common:status",
    ],
)

# Currently the GPU delegate needs to be built on Android (due to EGL dependency),
# or built with -DCL_DELEGATE_NO_GL (disabling OpenGL backend fallback), or both.
selects.config_setting_group(
    name = "supports_gpu_delegate",
    match_any = [
        "//tensorflow:android",
        "//tensorflow/lite/delegates/gpu/cl:opencl_delegate_no_gl",
    ],
)

cc_library(
    name = "delegate_options",
    srcs = ["delegate_options.cc"],
    hdrs = ["delegate_options.h"],
    deps = ["//tensorflow/lite/core/c:common"],
)

# copybara:uncomment_begin(google-only)
# cc_library(
#     name = "delegate_no_gl",
#     srcs = [
#         # copybara:comment_begin(oss-only)
# "android_version.cc",
#         # copybara:comment_end
#         "delegate.cc",
#     ],
#     hdrs = ["delegate.h"],
#     defines = ["CL_DELEGATE_NO_GL"],
#     linkopts = gpu_delegate_linkopts(),
#     deps = _DELEGATE_NO_GL_DEPS + [
#         "//tensorflow/lite/delegates/gpu/cl:api_no_gl",
#         "//tensorflow/lite/delegates/gpu/gl:api2",
#     ],
# )
# copybara:uncomment_end

cc_library(
    name = "delegate",
    srcs = [
        # copybara:comment_begin(oss-only)
        "android_version.cc",
        # copybara:comment_end
        "delegate.cc",
    ],
    hdrs = ["delegate.h"],
    linkopts = gpu_delegate_linkopts(),
    deps = select({
        "//tensorflow/lite/delegates/gpu/cl:opencl_delegate_no_gl": [],
        "//conditions:default": [
            "//tensorflow/lite/delegates/gpu/gl:api2",
        ],
    }) + _DELEGATE_NO_GL_DEPS + ["//tensorflow/lite/delegates/gpu/cl:api"],
)

cc_library(
    name = "tflite_profile",
    srcs = ["tflite_profile.cc"],
    hdrs = ["tflite_profile.h"],
    deps = [
        "//tensorflow/lite/core/api",
        "//tensorflow/lite/delegates/gpu/common/task:profiling_info",
        "@com_google_absl//absl/time",
    ],
)

cc_library(
    name = "android_hardware_buffer",
    srcs = ["android_hardware_buffer.cc"],
    hdrs = ["android_hardware_buffer.h"],
)

cc_test(
    name = "android_hardware_buffer_test",
    srcs = ["android_hardware_buffer_test.cc"],
    deps = [
        ":android_hardware_buffer",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "async_buffers",
    srcs = ["async_buffers.cc"],
    hdrs = ["async_buffers.h"],
    deps = [
        ":android_hardware_buffer",
        ":api",
        "//tensorflow/lite/delegates/gpu/common:data_type",
        "//tensorflow/lite/delegates/gpu/gl:gl_errors",
        "@com_google_absl//absl/status",
    ],
)

cc_test(
    name = "async_buffers_test",
    srcs = ["async_buffers_test.cc"],
    tags = tf_gpu_tests_tags() + [
        "local",
        "nobuilder",
        "notap",
        "tflite_not_portable_ios",
    ],
    deps = [
        ":android_hardware_buffer",
        ":async_buffers",
        ":delegate",
        "@com_google_googletest//:gtest_main",
    ],
)

tflite_portable_test_suite()
