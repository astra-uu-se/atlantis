#include "invariantgraph/constraints/intLinEqNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<IntLinEqNode> IntLinEqNode::fromModelConstraint(
    const fznparser::Model&, const fznparser::Constraint& constraint,
    InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  std::vector<Int> coeffs =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0))
          .toParVector();

  std::vector<VariableNode*> variables = invariantGraph.addVariableArray(
      std::get<fznparser::IntVarArray>(constraint.arguments().at(1)));

  Int bound =
      std::get<Int>(std::get<fznparser::IntArg>(constraint.arguments().at(2)));

  if (constraint.arguments().size() == 4) {
    const fznparser::BoolArg& reified =
        std::get<fznparser::BoolArg>(constraint.arguments().at(3));

    if (std::holds_alternative<bool>(reified)) {
      return std::make_unique<IntLinEqNode>(coeffs, variables, bound,
                                            std::get<bool>(reified));
    } else {
      return std::make_unique<IntLinEqNode>(
          coeffs, variables, bound,
          invariantGraph.addVariable(
              std::get<std::reference_wrapper<const fznparser::BoolVar>>(
                  reified)
                  .get()));
    }
  }
  return std::make_unique<IntLinEqNode>(coeffs, variables, bound, true);
}

void IntLinEqNode::createDefinedVariables(Engine& engine) {
  if (violationVarId() == NULL_ID) {
    _sumVarId = engine.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(engine.makeIntView<EqualConst>(engine, _sumVarId, _c));
    } else {
      assert(!isReified());
      setViolationVarId(
          engine.makeIntView<NotEqualConst>(engine, _sumVarId, _c));
    }
  }
}

void IntLinEqNode::registerWithEngine(Engine& engine) {
  std::vector<VarId> variables;
  std::transform(staticInputs().begin(), staticInputs().end(),
                 std::back_inserter(variables),
                 [&](auto node) { return node->varId(); });

  assert(_sumVarId != NULL_ID);
  assert(violationVarId() != NULL_ID);

  engine.makeInvariant<Linear>(engine, _sumVarId, _coeffs, variables);
}

}  // namespace invariantgraph