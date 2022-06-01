#include "invariantgraph/views/intAbsNode.hpp"

#include "../parseHelper.hpp"
#include "views/intAbsView.hpp"

std::unique_ptr<invariantgraph::IntAbsNode>
invariantgraph::IntAbsNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);

  return std::make_unique<invariantgraph::IntAbsNode>(a, b);
}

void invariantgraph::IntAbsNode::createDefinedVariables(Engine& engine) {
  if (definedVariables().front()->varId() == NULL_ID) {
    definedVariables().front()->setVarId(
        engine.makeIntView<IntAbsView>(input()->varId()));
  }
}

void invariantgraph::IntAbsNode::registerWithEngine(Engine&) {}
