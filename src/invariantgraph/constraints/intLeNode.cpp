#include "invariantgraph/constraints/intLeNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::IntLeNode>
invariantgraph::IntLeNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);

  if (constraint.arguments.size() >= 3) {
    if (std::holds_alternative<bool>(constraint.arguments[2])) {
      auto shouldHold = std::get<bool>(constraint.arguments[2]);
      return std::make_unique<invariantgraph::IntLeNode>(a, b, shouldHold);
    } else {
      auto r = mappedVariable(constraint.arguments[2], variableMap);
      return std::make_unique<invariantgraph::IntLeNode>(a, b, r);
    }
  }
  return std::make_unique<IntLeNode>(a, b, true);
}

void invariantgraph::IntLeNode::createDefinedVariables(Engine& engine) {
  registerViolation(engine);
}

void invariantgraph::IntLeNode::registerWithEngine(Engine& engine) {
  assert(violationVarId() != NULL_ID);

  if (shouldHold()) {
    engine.makeConstraint<LessEqual>(violationVarId(), a()->inputVarId(),
                                     b()->inputVarId());
  } else {
    assert(!isReified());
    engine.makeConstraint<LessThan>(violationVarId(), b()->inputVarId(),
                                    a()->inputVarId());
  }
}