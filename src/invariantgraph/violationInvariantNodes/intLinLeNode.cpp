#include "invariantgraph/violationInvariantNodes/intLinLeNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

IntLinLeNode::IntLinLeNode(std::vector<Int>&& coeffs,
                           std::vector<VarNodeId>&& variables, Int bound,
                           VarNodeId r)
    : ViolationInvariantNode(std::move(variables), r),
      _coeffs(std::move(coeffs)),
      _bound(bound) {}

IntLinLeNode::IntLinLeNode(std::vector<Int>&& coeffs,
                           std::vector<VarNodeId>&& variables, Int bound,
                           bool shouldHold)
    : ViolationInvariantNode(std::move(variables), shouldHold),
      _coeffs(std::move(coeffs)),
      _bound(bound) {}

std::unique_ptr<IntLinLeNode> IntLinLeNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  std::vector<Int> coeffs =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0))
          .toParVector();

  std::vector<VarNodeId> variables = invariantGraph.createVarNodes(
      std::get<fznparser::IntVarArray>(constraint.arguments().at(1)));

  Int bound =
      std::get<Int>(std::get<fznparser::IntArg>(constraint.arguments().at(2)));

  if (constraint.arguments().size() == 4) {
    const fznparser::BoolArg& reified =
        std::get<fznparser::BoolArg>(constraint.arguments().back());

    if (reified.isFixed()) {
      return std::make_unique<IntLinLeNode>(std::move(coeffs),
                                            std::move(variables), bound,
                                            reified.toParameter());
    } else {
      return std::make_unique<IntLinLeNode>(
          std::move(coeffs), std::move(variables), bound,
          invariantGraph.createVarNode(reified.var()));
    }
  }
  return std::make_unique<IntLinLeNode>(std::move(coeffs), std::move(variables),
                                        bound, true);
}

void IntLinLeNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                           propagation::SolverBase& solver) {
  if (violationVarId(invariantGraph) == propagation::NULL_ID) {
    _sumVarId = solver.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(invariantGraph, solver.makeIntView<propagation::LessEqualConst>(
                                            solver, _sumVarId, _bound));
    } else {
      assert(!isReified());
      setViolationVarId(invariantGraph, solver.makeIntView<propagation::GreaterEqualConst>(
                                            solver, _sumVarId, _bound + 1));
    }
  }
}

void IntLinLeNode::registerNode(InvariantGraph& invariantGraph,
                                propagation::SolverBase& solver) {
  std::vector<propagation::VarId> variables;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(variables),
                 [&](const auto& id) { return invariantGraph.varId(id); });

  assert(_sumVarId != propagation::NULL_ID);
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);

  solver.makeInvariant<propagation::Linear>(solver, _sumVarId, _coeffs, variables);
}

}  // namespace invariantgraph