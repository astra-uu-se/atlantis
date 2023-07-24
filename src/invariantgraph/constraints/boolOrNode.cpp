#include "invariantgraph/constraints/boolOrNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<BoolOrNode> BoolOrNode::fromModelConstraint(
    const fznparser::Model&, const fznparser::Constraint& constraint,
    InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  if (constraint.arguments().size() != 2 ||
      constraint.arguments().size() != 3) {
    throw std::runtime_error("BoolOr constraint takes two var bool arguments");
  }
  for (const auto& arg : constraint.arguments()) {
    if (!std::holds_alternative<fznparser::BoolArg>(arg)) {
      throw std::runtime_error(
          "BoolOr constraint takes two var bool arguments");
    }
  }

  VariableNode* a = invariantGraph.addVariable(
      std::get<fznparser::BoolArg>(constraint.arguments().at(0)));

  VariableNode* b = invariantGraph.addVariable(
      std::get<fznparser::BoolArg>(constraint.arguments().at(1)));

  if (constraint.arguments().size() == 2) {
    return std::make_unique<BoolOrNode>(a, b, true);
  }

  const auto& reified = get<fznparser::BoolArg>(constraint.arguments().at(2));
  if (std::holds_alternative<bool>(reified)) {
    return std::make_unique<BoolOrNode>(a, b, std::get<bool>(reified));
  }
  return std::make_unique<BoolOrNode>(
      a, b,
      invariantGraph.addVariable(
          std::get<std::reference_wrapper<const fznparser::BoolVar>>(reified)
              .get()));
}

void BoolOrNode::createDefinedVariables(Engine& engine) {
  if (violationVarId() == NULL_ID) {
    if (shouldHold()) {
      registerViolation(engine);
    } else {
      assert(!isReified());
      _intermediate = engine.makeIntVar(0, 0, 0);
      setViolationVarId(
          engine.makeIntView<NotEqualConst>(engine, _intermediate, 0));
    }
  }
}

void BoolOrNode::registerWithEngine(Engine& engine) {
  assert(violationVarId() != NULL_ID);

  engine.makeInvariant<BoolOr>(engine, violationVarId(), a()->varId(),
                               b()->varId());
}

}  // namespace invariantgraph