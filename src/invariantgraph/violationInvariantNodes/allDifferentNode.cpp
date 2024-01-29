#include "invariantgraph/violationInvariantNodes/allDifferentNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

AllDifferentNode::AllDifferentNode(std::vector<VarNodeId>&& vars, VarNodeId r)
    : ViolationInvariantNode(std::move(vars), r) {}

AllDifferentNode::AllDifferentNode(std::vector<VarNodeId>&& vars,
                                   bool shouldHold)
    : ViolationInvariantNode(std::move(vars), shouldHold) {}

void AllDifferentNode::registerOutputVars(InvariantGraph& invariantGraph,
                                          propagation::SolverBase& solver) {
  if (staticInputVarNodeIds().empty()) {
    return;
  }
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

bool AllDifferentNode::prune(InvariantGraph& invariantGraph) {
  if (isReified() || !shouldHold()) {
    return false;
  }

  std::vector<VarNodeId> fixedInputs =
      pruneAllDifferentFixed(invariantGraph, staticInputVarNodeIds());

  for (const auto& fixedVarNodeId : fixedInputs) {
    removeStaticInputVarNode(invariantGraph.varNode(fixedVarNodeId));
  }

  return !fixedInputs.empty();
}

void AllDifferentNode::registerNode(InvariantGraph& invariantGraph,
                                    propagation::SolverBase& solver) {
  if (staticInputVarNodeIds().empty()) {
    return;
  }
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);

  std::vector<propagation::VarId> solverVars;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars),
                 [&](const auto& id) { return invariantGraph.varId(id); });

  solver.makeViolationInvariant<propagation::AllDifferent>(
      solver, !shouldHold() ? _intermediate : violationVarId(invariantGraph),
      solverVars);
}

}  // namespace atlantis::invariantgraph