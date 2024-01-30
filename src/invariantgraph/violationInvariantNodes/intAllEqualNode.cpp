#include "invariantgraph/violationInvariantNodes/intAllEqualNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

IntAllEqualNode::IntAllEqualNode(std::vector<VarNodeId>&& vars, VarNodeId r)
    : ViolationInvariantNode(std::move(vars), r) {}

IntAllEqualNode::IntAllEqualNode(std::vector<VarNodeId>&& vars, bool shouldHold)
    : ViolationInvariantNode(std::move(vars), shouldHold) {}

void IntAllEqualNode::registerOutputVars(InvariantGraph& invariantGraph,
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

void IntAllEqualNode::registerNode(InvariantGraph& invariantGraph,
                                   propagation::SolverBase& solver) {
  assert(_allDifferentViolationVarId != propagation::NULL_ID);
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);

  std::vector<propagation::VarId> inputVarIds;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(inputVarIds),
                 [&](const auto& id) { return invariantGraph.varId(id); });

  solver.makeViolationInvariant<propagation::AllDifferent>(
      solver, _allDifferentViolationVarId, std::move(inputVarIds));
}

}  // namespace atlantis::invariantgraph