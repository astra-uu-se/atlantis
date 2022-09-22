#include "invariantgraph/constraints/boolXorNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::BoolXorNode>
invariantgraph::BoolXorNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);

  if (constraint.arguments.size() >= 3) {
    if (std::holds_alternative<bool>(constraint.arguments[2])) {
      auto shouldHold = std::get<bool>(constraint.arguments[2]);
      return std::make_unique<invariantgraph::BoolXorNode>(a, b, shouldHold);
    } else {
      auto r = mappedVariable(constraint.arguments[2], variableMap);
      return std::make_unique<invariantgraph::BoolXorNode>(a, b, r);
    }
  }
  return std::make_unique<BoolXorNode>(a, b, true);
}

void invariantgraph::BoolXorNode::createDefinedVariables(Engine& engine) {
  registerViolation(engine);
}

void invariantgraph::BoolXorNode::registerWithEngine(Engine& engine) {
  assert(violationVarId() != NULL_ID);
  assert(a()->inputVarId() != NULL_ID);
  assert(b()->inputVarId() != NULL_ID);

  if (shouldHold()) {
    engine.makeInvariant<BoolXor>(violationVarId(), a()->inputVarId(),
                                  b()->inputVarId());
  } else {
    assert(!isReified());
    engine.makeConstraint<BoolEqual>(violationVarId(), a()->inputVarId(),
                                     b()->inputVarId());
  }
}