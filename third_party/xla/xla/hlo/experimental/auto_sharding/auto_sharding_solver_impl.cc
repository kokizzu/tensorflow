/* Copyright 2023 The OpenXLA Authors.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include <cstdint>
#include <vector>

#include "xla/hlo/experimental/auto_sharding/auto_sharding.pb.h"
#include "xla/hlo/experimental/auto_sharding/auto_sharding_solver.h"
#include "xla/hlo/experimental/auto_sharding/auto_sharding_strategy.h"
#include "xla/hlo/experimental/auto_sharding/iopddl.h"
#include "ortools/linear_solver/linear_solver.h"

namespace xla {
namespace spmd {

using ::operations_research::MPSolver;
using ::operations_research::MPVariable;

MPVariable* CreateMakespanVar(const AutoShardingSolverRequest& request,
                              const std::vector<std::vector<MPVariable*>>& e,
                              MPSolver& solver) {
  return nullptr;  // TODO(moffitt): Implement this.
}

double EvaluateMakespan(const AutoShardingSolverRequest& request,
                        const AutoShardingSolverOutput& result,
                        AutoShardingEvaluation& evaluation) {
  return 0.0;  // TODO(moffitt): Implement this.
}

StrategyShaverForRequest::StrategyShaverForRequest(
    const AutoShardingSolverParams& params,
    const AutoShardingSolverRequest& request)
    : params_(params), request_(request) {}

NodeStrategies StrategyShaverForRequest::FindShavedStrategies() const {
  return {};  // TODO(moffitt): Implement this.
}

StrategyShaverForProblem::StrategyShaverForProblem(
    const AutoShardingSolverParams& params, const iopddl::Problem& problem,
    const std::vector<int64_t>& s_follow,
    const std::vector<iopddl::Edge>& aliases,
    const std::vector<iopddl::Edge>& deduplicated_edges)
    : params_(params),
      problem_(problem),
      s_follow_(s_follow),
      aliases_(aliases),
      deduplicated_edges_(deduplicated_edges) {}

NodeStrategies StrategyShaverForProblem::FindShavedStrategies() const {
  return {};  // TODO(moffitt): Implement this.
}

void SolverRequestCallback(const AutoShardingSolverRequest& request) {
  // TODO(mofftt): Implement this.
}

AutoShardingSolverOutput SolveBrkga(const AutoShardingSolverRequest& request) {
  // TODO(fahrbach): Implement this.
  AutoShardingSolverOutput output;
  return output;
}

}  // namespace spmd
}  // namespace xla
