#include "invariantgraph/violationInvariantNodes/boolAllEqualNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

BoolAllEqualNode::BoolAllEqualNode(std::vector<VarNodeId>&& vars, VarNodeId r)
    : ViolationInvariantNode(std::move(vars), r) {}

BoolAllEqualNode::BoolAllEqualNode(std::vector<VarNodeId>&& vars,
                                   bool shouldHold)
    : ViolationInvariantNode(std::move(vars), shouldHold) {}

bool BoolAllEqualNode::prune(InvariantGraph& invariantGraph) {
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

void BoolAllEqualNode::registerOutputVars(InvariantGraph& invariantGraph,
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

void BoolAllEqualNode::registerNode(InvariantGraph& invariantGraph,
                                    propagation::SolverBase& solver) {
  if (staticInputVarNodeIds().empty()) {
    return;
  }
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);

  std::vector<propagation::VarId> solverVars;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars),
                 [&](const auto& id) { return invariantGraph.varId(id); });

  solver.makeViolationInvariant<propagation::BoolAllEqual>(
      solver, !shouldHold() ? _intermediate : violationVarId(invariantGraph),
      solverVars);
}

}  // namespace atlantis::invariantgraph