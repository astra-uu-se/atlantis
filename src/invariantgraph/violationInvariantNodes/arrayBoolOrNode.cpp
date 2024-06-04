#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolOrNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/exists.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {

ArrayBoolOrNode::ArrayBoolOrNode(std::vector<VarNodeId>&& inputs,
                                 VarNodeId output)
    : ViolationInvariantNode(std::move(inputs), output) {}

ArrayBoolOrNode::ArrayBoolOrNode(std::vector<VarNodeId>&& inputs,
                                 bool shouldHold)
    : ViolationInvariantNode(std::move(inputs), shouldHold) {}

void ArrayBoolOrNode::registerOutputVars(InvariantGraph& invariantGraph,
                                         propagation::SolverBase& solver) {
  if (violationVarId(invariantGraph) == propagation::NULL_ID) {
    if (shouldHold()) {
      registerViolation(invariantGraph, solver);
    } else {
      assert(!isReified());
      _intermediate = solver.makeIntVar(0, 0, 0);
      setViolationVarId(invariantGraph,
                        solver.makeIntView<propagation::NotEqualConst>(
                            solver, _intermediate, 0));
    }
  }
}

void ArrayBoolOrNode::registerNode(InvariantGraph& invariantGraph,
                                   propagation::SolverBase& solver) {
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);
  std::vector<propagation::VarId> inputVarIds;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(inputVarIds),
                 [&](const auto& node) { return invariantGraph.varId(node); });

  solver.makeInvariant<propagation::Exists>(
      solver, !shouldHold() ? _intermediate : violationVarId(invariantGraph),
      std::move(inputVarIds));
}

}  // namespace atlantis::invariantgraph
