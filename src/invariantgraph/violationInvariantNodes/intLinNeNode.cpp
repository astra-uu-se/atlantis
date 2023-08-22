#include "invariantgraph/violationInvariantNodes/intLinNeNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<IntLinNeNode> IntLinNeNode::fromModelConstraint(
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
      return std::make_unique<IntLinNeNode>(std::move(coeffs),
                                            std::move(variables), bound,
                                            reified.toParameter());
    } else {
      return std::make_unique<IntLinNeNode>(
          std::move(coeffs), std::move(variables), bound,
          invariantGraph.createVarNode(
              std::get<std::reference_wrapper<const fznparser::BoolVar>>(
                  reified)
                  .get()));
    }
  }
  return std::make_unique<IntLinNeNode>(std::move(coeffs), std::move(variables),
                                        bound, true);
}

void IntLinNeNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                           Engine& engine) {
  if (_sumVarId == NULL_ID) {
    _sumVarId = engine.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(
          engine.makeIntView<NotEqualConst>(engine, _sumVarId, _c));
    } else {
      assert(!isReified());
      setViolationVarId(engine.makeIntView<EqualConst>(engine, _sumVarId, _c));
    }
  }
}

void IntLinNeNode::registerNode(InvariantGraph& invariantGraph,
                                Engine& engine) {
  std::vector<VarId> variables;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(variables),
                 [&](auto node) { return node->varId(); });

  assert(_sumVarId != NULL_ID);
  assert(violationVarId() != NULL_ID);

  engine.makeInvariant<Linear>(engine, _sumVarId, _coeffs, variables);
}

}  // namespace invariantgraph