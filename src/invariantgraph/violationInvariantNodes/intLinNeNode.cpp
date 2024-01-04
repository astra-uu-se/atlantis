#include "invariantgraph/violationInvariantNodes/intLinNeNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

IntLinNeNode::IntLinNeNode(std::vector<Int>&& coeffs,
                           std::vector<VarNodeId>&& vars, Int c, VarNodeId r)
    : ViolationInvariantNode(std::move(vars), r),
      _coeffs(std::move(coeffs)),
      _c(c) {}

IntLinNeNode::IntLinNeNode(std::vector<Int>&& coeffs,
                           std::vector<VarNodeId>&& vars, Int c,
                           bool shouldHold)
    : ViolationInvariantNode(std::move(vars), shouldHold),
      _coeffs(std::move(coeffs)),
      _c(c) {}

void IntLinNeNode::registerOutputVars(InvariantGraph& invariantGraph,
                                      propagation::SolverBase& solver) {
  if (_sumVarId == propagation::NULL_ID) {
    _sumVarId = solver.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(invariantGraph,
                        solver.makeIntView<propagation::NotEqualConst>(
                            solver, _sumVarId, _c));
    } else {
      assert(!isReified());
      setViolationVarId(
          invariantGraph,
          solver.makeIntView<propagation::EqualConst>(solver, _sumVarId, _c));
    }
  }
}

void IntLinNeNode::registerNode(InvariantGraph& invariantGraph,
                                propagation::SolverBase& solver) {
  std::vector<propagation::VarId> solverVars;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars),
                 [&](const auto& id) { return invariantGraph.varId(id); });

  assert(_sumVarId != propagation::NULL_ID);
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);

  solver.makeInvariant<propagation::Linear>(solver, _sumVarId, _coeffs,
                                            solverVars);
}

}  // namespace atlantis::invariantgraph