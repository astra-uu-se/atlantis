#include "invariantgraph/violationInvariantNodes/intLinEqNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

IntLinEqNode::IntLinEqNode(std::vector<Int>&& coeffs,
                           std::vector<VarNodeId>&& variables, Int c,
                           VarNodeId r)
    : ViolationInvariantNode(std::move(variables), r),
      _coeffs(std::move(coeffs)),
      _c(c) {}

IntLinEqNode::IntLinEqNode(std::vector<Int>&& coeffs,
                           std::vector<VarNodeId>&& variables, Int c,
                           bool shouldHold)
    : ViolationInvariantNode(std::move(variables), shouldHold),
      _coeffs(std::move(coeffs)),
      _c(c) {}

std::unique_ptr<IntLinEqNode> IntLinEqNode::fromModelConstraint(
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
      return std::make_unique<IntLinEqNode>(std::move(coeffs),
                                            std::move(variables), bound,
                                            reified.toParameter());
    } else {
      return std::make_unique<IntLinEqNode>(
          std::move(coeffs), std::move(variables), bound,
          invariantGraph.createVarNode(reified.var()));
    }
  }
  return std::make_unique<IntLinEqNode>(std::move(coeffs), std::move(variables),
                                        bound, true);
}

void IntLinEqNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                           Engine& engine) {
  if (violationVarId(invariantGraph) == NULL_ID) {
    _sumVarId = engine.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(invariantGraph,
                        engine.makeIntView<EqualConst>(engine, _sumVarId, _c));
    } else {
      assert(!isReified());
      setViolationVarId(invariantGraph, engine.makeIntView<NotEqualConst>(
                                            engine, _sumVarId, _c));
    }
  }
}

void IntLinEqNode::registerNode(InvariantGraph& invariantGraph,
                                Engine& engine) {
  std::vector<VarId> variables;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(variables),
                 [&](const auto& id) { return invariantGraph.varId(id); });

  assert(_sumVarId != NULL_ID);
  assert(violationVarId(invariantGraph) != NULL_ID);

  engine.makeInvariant<Linear>(engine, _sumVarId, _coeffs, variables);
}

}  // namespace invariantgraph