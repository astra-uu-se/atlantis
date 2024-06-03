#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolAndNode.hpp"

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/forAll.hpp"
#include "atlantis/propagation/views/elementConst.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {

ArrayBoolAndNode::ArrayBoolAndNode(std::vector<VarNodeId>&& as,
                                   VarNodeId output)
    : ViolationInvariantNode(std::move(as), output) {}

ArrayBoolAndNode::ArrayBoolAndNode(std::vector<VarNodeId>&& as, bool shouldHold)
    : ViolationInvariantNode(std::move(as), shouldHold) {}

void ArrayBoolAndNode::registerOutputVars(InvariantGraph& invariantGraph,
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

void ArrayBoolAndNode::propagate(InvariantGraph& invariantGraph) {
  ViolationInvariantNode::propagate(invariantGraph);
  std::vector<VarNodeId> fixedInputs;
  fixedInputs.reserve(staticInputVarNodeIds().size());
  bool any_false = false;
  bool all_true = true;
  for (const auto& id : staticInputVarNodeIds()) {
    if (invariantGraph.isFixed(id)) {
      fixedInputs.emplace_back(id);
      any_false = any_false || invariantGraph.lowerBound(id) == 0;
      all_true = all_true && invariantGraph.lowerBound(id) > 0;
    }
  }
  for (const auto& fixedVarNodeId : fixedInputs) {
    removeStaticInputVarNode(invariantGraph.varNode(fixedVarNodeId));
  }
  if (any_false) {
    if (isReified()) {
      invariantGraph.fixToValue(_reifiedViolationNodeId, false);
      ViolationInvariantNode::propagate(invariantGraph);
    }
    setState(shouldHold() ? InvariantNodeState::INFEASIBLE
                          : InvariantNodeState::SUBSUMED);
  } else if (all_true) {
    if (isReified()) {
      invariantGraph.fixToValue(_reifiedViolationNodeId, true);
      ViolationInvariantNode::propagate(invariantGraph);
    }
    setState(shouldHold() ? InvariantNodeState::SUBSUMED
                          : InvariantNodeState::INFEASIBLE);
  }
}

std::vector<VarNodeId> fixedInputs =
    pruneAllDifferentFixed(invariantGraph, staticInputVarNodeIds());

for (const auto& fixedVarNodeId : fixedInputs) {
  removeStaticInputVarNode(invariantGraph.varNode(fixedVarNodeId));
}

if (staticInputVarNodeIds().size() < 2) {
  setState(InvariantNodeState::SUBSUMED);
}
}

void ArrayBoolAndNode::registerNode(InvariantGraph& invariantGraph,
                                    propagation::SolverBase& solver) {
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);
  std::vector<propagation::VarId> inputVarIds;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(inputVarIds),
                 [&](const auto& node) { return invariantGraph.varId(node); });

  solver.makeInvariant<propagation::ForAll>(
      solver, !shouldHold() ? _intermediate : violationVarId(invariantGraph),
      std::move(inputVarIds));
}

}  // namespace atlantis::invariantgraph
