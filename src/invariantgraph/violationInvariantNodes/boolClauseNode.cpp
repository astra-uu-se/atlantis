#include "invariantgraph/violationInvariantNodes/boolClauseNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

BoolClauseNode::BoolClauseNode(std::vector<VarNodeId>&& as,
                               std::vector<VarNodeId>&& bs, VarNodeId r)
    : ViolationInvariantNode(std::move(concat(as, bs)), r),
      _as(std::move(as)),
      _bs(std::move(bs)) {}
BoolClauseNode::BoolClauseNode(std::vector<VarNodeId>&& as,
                               std::vector<VarNodeId>&& bs, bool shouldHold)
    : ViolationInvariantNode(std::move(concat(as, bs)), shouldHold),
      _as(std::move(as)),
      _bs(std::move(bs)) {}

void BoolClauseNode::registerOutputVars(InvariantGraph& invariantGraph,
                                        propagation::SolverBase& solver) {
  if (violationVarId(invariantGraph) == propagation::NULL_ID) {
    _sumVarId = solver.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(
          invariantGraph,
          solver.makeIntView<propagation::EqualConst>(
              solver, _sumVarId,
              static_cast<Int>(_as.size()) + static_cast<Int>(_bs.size())));
    } else {
      assert(!isReified());
      setViolationVarId(
          invariantGraph,
          solver.makeIntView<propagation::NotEqualConst>(
              solver, _sumVarId,
              static_cast<Int>(_as.size()) + static_cast<Int>(_bs.size())));
    }
  }
}

void BoolClauseNode::registerNode(InvariantGraph& invariantGraph,
                                  propagation::SolverBase& solver) {
  std::vector<propagation::VarId> solverVars;
  solverVars.reserve(_as.size() + _bs.size());
  std::transform(_as.begin(), _as.end(), std::back_inserter(solverVars),
                 [&](const auto& id) { return invariantGraph.varId(id); });

  std::transform(_bs.begin(), _bs.end(), std::back_inserter(solverVars),
                 [&](const auto& id) {
                   return solver.makeIntView<propagation::NotEqualConst>(
                       solver, invariantGraph.varId(id), 0);
                 });

  assert(_sumVarId != propagation::NULL_ID);
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);
  solver.makeInvariant<propagation::BoolLinear>(solver, _sumVarId, solverVars);
}

}  // namespace atlantis::invariantgraph