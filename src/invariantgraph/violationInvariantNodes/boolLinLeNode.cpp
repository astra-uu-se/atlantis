#include "invariantgraph/violationInvariantNodes/boolLinLeNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

BoolLinLeNode::BoolLinLeNode(std::vector<Int> coeffs,
                             std::vector<VarNodeId>&& vars, Int bound,
                             VarNodeId r)
    : ViolationInvariantNode(std::move(vars), r),
      _coeffs(std::move(coeffs)),
      _bound(bound) {}

BoolLinLeNode::BoolLinLeNode(std::vector<Int> coeffs,
                             std::vector<VarNodeId>&& vars, Int bound,
                             bool shouldHold)
    : ViolationInvariantNode(std::move(vars), shouldHold),
      _coeffs(std::move(coeffs)),
      _bound(bound) {}

void BoolLinLeNode::registerOutputVars(InvariantGraph& invariantGraph,
                                       propagation::SolverBase& solver) {
  if (violationVarId(invariantGraph) == propagation::NULL_ID) {
    _sumVarId = solver.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(invariantGraph,
                        solver.makeIntView<propagation::LessEqualConst>(
                            solver, _sumVarId, _bound));
    } else {
      assert(!isReified());
      setViolationVarId(invariantGraph,
                        solver.makeIntView<propagation::GreaterEqualConst>(
                            solver, _sumVarId, _bound + 1));
    }
  }
}

void BoolLinLeNode::registerNode(InvariantGraph& invariantGraph,
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