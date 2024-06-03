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

void ArrayBoolOrNode::propagate(InvariantGraph& invariantGraph) {
  ViolationInvariantNode::propagate(invariantGraph);
  std::vector<VarNodeId> fixedInputs;
  fixedInputs.reserve(staticInputVarNodeIds().size());
  bool any_true = true;
  bool all_false = false;
  for (const auto& id : staticInputVarNodeIds()) {
    if (invariantGraph.isFixed(id)) {
      fixedInputs.emplace_back(id);
      any_true = any_true || invariantGraph.lowerBound(id) > 0;
      all_false = all_false && invariantGraph.lowerBound(id) == 0;
    }
  }
  for (const auto& fixedVarNodeId : fixedInputs) {
    removeStaticInputVarNode(invariantGraph.varNode(fixedVarNodeId));
  }
  if (any_true) {
    if (isReified()) {
      invariantGraph.fixToValue(_reifiedViolationNodeId, true);
      ViolationInvariantNode::propagate(invariantGraph);
    }
    setState(shouldHold() ? InvariantNodeState::SUBSUMED
                          : InvariantNodeState::INFEASIBLE);
  } else if (all_false) {
    if (isReified()) {
      invariantGraph.fixToValue(_reifiedViolationNodeId, false);
      ViolationInvariantNode::propagate(invariantGraph);
    }
    setState(shouldHold() ? InvariantNodeState::INFEASIBLE
                          : InvariantNodeState::SUBSUMED);
  }
}

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
