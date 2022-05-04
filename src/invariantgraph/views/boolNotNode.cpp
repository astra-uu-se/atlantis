#include "invariantgraph/views/boolNotNode.hpp"

#include "../parseHelper.hpp"
#include "views/bool2IntView.hpp"

std::unique_ptr<invariantgraph::BoolNotNode>
invariantgraph::BoolNotNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);

  return std::make_unique<invariantgraph::BoolNotNode>(a, b);
}

void invariantgraph::BoolNotNode::createDefinedVariables(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  auto outputVar = engine.makeIntView<::Bool2IntView>(variableMap.at(_input));
  variableMap.emplace(definedVariables()[0], outputVar);
}

void invariantgraph::BoolNotNode::registerWithEngine(
    Engine&, VariableDefiningNode::VariableMap&) {}
