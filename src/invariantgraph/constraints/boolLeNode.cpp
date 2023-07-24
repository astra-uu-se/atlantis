#include "invariantgraph/constraints/boolLeNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::BoolLeNode>
invariantgraph::BoolLeNode::fromModelConstraint(
    const fznparser::Model&, const fznparser::Constraint& constraint,
    InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  if (constraint.arguments().size() != 2 ||
      constraint.arguments().size() != 3) {
    throw std::runtime_error("BoolLe constraint takes two var bool arguments");
  }
  for (const auto& arg : constraint.arguments()) {
    if (!std::holds_alternative<fznparser::BoolArg>(arg)) {
      throw std::runtime_error(
          "BoolLe constraint takes two var bool arguments");
    }
  }
  VariableNode* a = invariantGraph.addVariable(
      std::get<fznparser::BoolArg>(constraint.arguments().at(0)));

  VariableNode* b = invariantGraph.addVariable(
      std::get<fznparser::BoolArg>(constraint.arguments().at(1)));

  if (constraint.arguments().size() == 2) {
    return std::make_unique<BoolLeNode>(a, b, true);
  }

  const auto& reified = get<fznparser::BoolArg>(constraint.arguments().at(2));
  if (std::holds_alternative<bool>(reified)) {
    return std::make_unique<invariantgraph::BoolLeNode>(
        a, b, std::get<bool>(reified));
  }
  return std::make_unique<invariantgraph::BoolLeNode>(
      a, b,
      invariantGraph.addVariable(
          std::get<std::reference_wrapper<const fznparser::BoolVar>>(reified)
              .get()));
}

void invariantgraph::BoolLeNode::createDefinedVariables(Engine& engine) {
  registerViolation(engine);
}

void invariantgraph::BoolLeNode::registerWithEngine(Engine& engine) {
  assert(violationVarId() != NULL_ID);
  assert(a()->varId() != NULL_ID);
  assert(b()->varId() != NULL_ID);

  if (shouldHold()) {
    engine.makeConstraint<BoolLessEqual>(engine, violationVarId(), a()->varId(),
                                         b()->varId());
  } else {
    assert(!isReified());
    engine.makeConstraint<BoolLessThan>(engine, violationVarId(), b()->varId(),
                                        a()->varId());
  }
}