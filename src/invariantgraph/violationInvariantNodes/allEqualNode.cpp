#include "invariantgraph/violationInvariantNodes/allEqualNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

AllEqualNode::AllEqualNode(std::vector<VarNodeId>&& vars, VarNodeId r)
    : ViolationInvariantNode(std::move(vars), r) {}

AllEqualNode::AllEqualNode(std::vector<VarNodeId>&& vars, bool shouldHold)
    : ViolationInvariantNode(std::move(vars), shouldHold) {}

bool AllEqualNode::canBeReplaced() const {
  return staticInputVarNodeIds().size() == 2;
}

bool AllEqualNode::replace(InvariantGraph& invariantGraph) {
  if (!canBeReplaced()) {
    throw InvariantGraphException(
        "AllEqualNode::replace() called on node that cannot be replaced");
  }
  const std::vector<VarNodeId> inputs = staticInputVarNodeIds();
  for (const VarNodeId input : inputs) {
    removeStaticInputVarNode(invariantGraph.varNode(input));
  }
  if (isReified()) {
    const VarNodeId reifNodeId = reifiedViolationNodeId();
    removeOutputVarNode(invariantGraph.varNode(reifNodeId));
    invariantGraph.addInvariantNode(
        std::make_unique<IntEqNode>(inputs.at(0), inputs.at(1), reifNodeId));
  } else {
    assert(outputVarNodeIds().empty());
    invariantGraph.addInvariantNode(
        std::make_unique<IntEqNode>(inputs.at(0), inputs.at(1), shouldHold()));
  }
}

bool AllEqualNode::canBeRemoved() const {
  return staticInputVarNodeIds().size() == 1;
}

void AllEqualNode::registerOutputVars(InvariantGraph& invariantGraph,
                                      propagation::SolverBase& solver) {
  if (violationVarId(invariantGraph) == propagation::NULL_ID) {
    _allDifferentViolationVarId = solver.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(invariantGraph,
                        solver.makeIntView<propagation::EqualConst>(
                            solver, _allDifferentViolationVarId,
                            staticInputVarNodeIds().size() - 1));
    } else {
      assert(!isReified());
      setViolationVarId(invariantGraph,
                        solver.makeIntView<propagation::NotEqualConst>(
                            solver, _allDifferentViolationVarId,
                            staticInputVarNodeIds().size() - 1));
    }
  }
}

void AllEqualNode::registerNode(InvariantGraph& invariantGraph,
                                propagation::SolverBase& solver) {
  assert(_allDifferentViolationVarId != propagation::NULL_ID);
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);

  std::vector<propagation::VarId> solverVars;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars),
                 [&](const auto& id) { return invariantGraph.varId(id); });

  solver.makeViolationInvariant<propagation::AllDifferent>(
      solver, _allDifferentViolationVarId, solverVars);
}

}  // namespace atlantis::invariantgraph