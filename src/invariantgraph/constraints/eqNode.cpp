#include "invariantgraph/constraints/eqNode.hpp"

#include "../parseHelper.hpp"
#include "constraints/equal.hpp"

std::unique_ptr<invariantgraph::EqNode>
invariantgraph::EqNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);

  return std::make_unique<EqNode>(a, b);
}

void invariantgraph::EqNode::createDefinedVariables(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  if (!variableMap.contains(violation())) {
    registerViolation(engine, variableMap);
  }
}

void invariantgraph::EqNode::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  assert(variableMap.contains(violation()));

  engine.makeConstraint<::Equal>(variableMap.at(violation()),
                                 variableMap.at(_a), variableMap.at(_b));
}