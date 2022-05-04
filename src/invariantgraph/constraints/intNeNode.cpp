#include "invariantgraph/constraints/intNeNode.hpp"

#include "../parseHelper.hpp"
#include "constraints/notEqual.hpp"

std::unique_ptr<invariantgraph::IntNeNode>
invariantgraph::IntNeNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "int_ne");
  assert(constraint.arguments.size() == 2);

  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);

  return std::make_unique<IntNeNode>(a, b);
}

void invariantgraph::IntNeNode::createDefinedVariables(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  if (!variableMap.contains(violation())) {
    registerViolation(engine, variableMap);
  }
}

void invariantgraph::IntNeNode::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  assert(variableMap.contains(violation()));

  engine.makeConstraint<::NotEqual>(variableMap.at(violation()),
                                    variableMap.at(_a), variableMap.at(_b));
}