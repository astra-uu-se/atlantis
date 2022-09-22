#include "invariantgraph/views/boolNotNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::BoolNotNode>
invariantgraph::BoolNotNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);

  return std::make_unique<invariantgraph::BoolNotNode>(a, b);
}

void invariantgraph::BoolNotNode::createDefinedVariables(Engine& engine) {
  if (definedVariables().front()->varId() == NULL_ID) {
    definedVariables().front()->setVarId(
        engine.makeIntView<Bool2IntView>(input()->varId(), this));
  }
}

void invariantgraph::BoolNotNode::registerWithEngine(Engine&) {}
