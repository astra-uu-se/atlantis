#include "invariantgraph/constraints/leNode.hpp"

#include "../parseHelper.hpp"
#include "constraints/lessEqual.hpp"

std::unique_ptr<invariantgraph::LeNode>
invariantgraph::LeNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(
      (constraint.name == "int_ne" && constraint.arguments.size() == 2) ||
      (constraint.name == "int_ne_reif" && constraint.arguments.size() == 3));

  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);

  VariableNode* r = constraint.arguments.size() >= 3
                        ? mappedVariable(constraint.arguments[2], variableMap)
                        : nullptr;

  return std::make_unique<LeNode>(a, b, r);
}

void invariantgraph::LeNode::createDefinedVariables(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  if (!variableMap.contains(violation())) {
    registerViolation(engine, variableMap);
  }
}

void invariantgraph::LeNode::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  assert(variableMap.contains(violation()));

  engine.makeConstraint<::LessEqual>(variableMap.at(violation()),
                                     variableMap.at(a()), variableMap.at(b()));
}