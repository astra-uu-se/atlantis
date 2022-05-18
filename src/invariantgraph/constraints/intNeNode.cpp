#include "invariantgraph/constraints/intNeNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::IntNeNode>
invariantgraph::IntNeNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(
      (constraint.name == "int_ne" && constraint.arguments.size() == 2) ||
      (constraint.name == "int_ne_reif" && constraint.arguments.size() == 3));

  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);

  if (constraint.arguments.size() >= 3) {
    if (std::holds_alternative<bool>(constraint.arguments[2])) {
      auto shouldHold = std::get<bool>(constraint.arguments[2]);
      return std::make_unique<invariantgraph::IntNeNode>(a, b, shouldHold);
    } else {
      auto r = mappedVariable(constraint.arguments[2], variableMap);
      return std::make_unique<invariantgraph::IntNeNode>(a, b, r);
    }
  }
  return std::make_unique<IntNeNode>(a, b, true);
}

void invariantgraph::IntNeNode::createDefinedVariables(Engine& engine) {
  registerViolation(engine);
}

void invariantgraph::IntNeNode::registerWithEngine(Engine& engine) {
  assert(violationVarId() != NULL_ID);

  if (shouldHold()) {
    engine.makeConstraint<NotEqual>(violationVarId(), a()->varId(),
                                    b()->varId());
  } else {
    assert(!isReified());
    engine.makeConstraint<Equal>(violationVarId(), a()->varId(), b()->varId());
  }
}