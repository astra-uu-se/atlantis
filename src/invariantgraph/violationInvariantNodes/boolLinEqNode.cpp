#include "invariantgraph/violationInvariantNodes/boolLinEqNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

BoolLinEqNode::BoolLinEqNode(std::vector<Int>&& coeffs,
                             std::vector<VarNodeId>&& vars, Int c, VarNodeId r)
    : ViolationInvariantNode(std::move(vars), r),
      _coeffs(std::move(coeffs)),
      _c(c) {}

BoolLinEqNode::BoolLinEqNode(std::vector<Int>&& coeffs,
                             std::vector<VarNodeId>&& vars, Int c,
                             bool shouldHold)
    : ViolationInvariantNode(std::move(vars), shouldHold),
      _coeffs(std::move(coeffs)),
      _c(c) {}

void BoolLinEqNode::registerOutputVars(InvariantGraph& invariantGraph,
                                       propagation::SolverBase& solver) {
  if (violationVarId(invariantGraph) == propagation::NULL_ID) {
    _sumVarId = solver.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(
          invariantGraph,
          solver.makeIntView<propagation::EqualConst>(solver, _sumVarId, _c));
    } else {
      setViolationVarId(invariantGraph,
                        solver.makeIntView<propagation::NotEqualConst>(
                            solver, _sumVarId, _c));
    }
  }
}

void BoolLinEqNode::registerNode(InvariantGraph& invariantGraph,
                                 propagation::SolverBase& solver) {
  std::vector<propagation::VarId> engineVars;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(engineVars),
                 [&](const auto& id) { return invariantGraph.varId(id); });

  assert(_sumVarId != propagation::NULL_ID);
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);

  solver.makeInvariant<propagation::BoolLinear>(solver, _sumVarId, _coeffs,
                                                engineVars);
}

}  // namespace atlantis::invariantgraph