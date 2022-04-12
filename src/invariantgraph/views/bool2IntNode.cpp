#include "invariantgraph/views/bool2IntNode.hpp"

#include "../parseHelper.hpp"
#include "views/bool2IntView.hpp"

std::unique_ptr<invariantgraph::Bool2IntNode>
invariantgraph::Bool2IntNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);

  return std::make_unique<invariantgraph::Bool2IntNode>(a, b);
}

void invariantgraph::Bool2IntNode::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  auto outputVar = engine.makeIntView<::Bool2IntView>(variableMap.at(_input));
  variableMap.emplace(definedVariables()[0], outputVar);
}
