#include "invariantgraph/violationInvariantNodes/intLinLeNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

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
                                           Engine& engine) {
  if (violationVarId() == NULL_ID) {
    _sumVarId = engine.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(
          engine.makeIntView<LessEqualConst>(engine, _sumVarId, _bound));
    } else {
      assert(!isReified());
      setViolationVarId(
          engine.makeIntView<GreaterEqualConst>(engine, _sumVarId, _bound + 1));
    }
  }
}

void IntLinLeNode::registerNode(InvariantGraph& invariantGraph,
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