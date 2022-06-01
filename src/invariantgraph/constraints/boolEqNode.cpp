#include "invariantgraph/constraints/boolEqNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::BoolEqNode>
invariantgraph::BoolEqNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);

  if (constraint.arguments.size() >= 3) {
    if (std::holds_alternative<bool>(constraint.arguments[2])) {
      auto shouldHold = std::get<bool>(constraint.arguments[2]);
      return std::make_unique<invariantgraph::BoolEqNode>(a, b, shouldHold);
    } else {
      auto r = mappedVariable(constraint.arguments[2], variableMap);
      return std::make_unique<invariantgraph::BoolEqNode>(a, b, r);
    }
  }
  return std::make_unique<BoolEqNode>(a, b, true);
}

void invariantgraph::BoolEqNode::createDefinedVariables(Engine& engine) {
  registerViolation(engine);
}

void invariantgraph::BoolEqNode::registerWithEngine(Engine& engine) {
  assert(violationVarId() != NULL_ID);
  assert(a()->varId() != NULL_ID);
  assert(b()->varId() != NULL_ID);

  if (shouldHold()) {
    engine.makeConstraint<BoolEqual>(violationVarId(), a()->varId(),
                                     b()->varId());
  } else {
    engine.makeInvariant<BoolXor>(violationVarId(), a()->varId(), b()->varId());
  }
}