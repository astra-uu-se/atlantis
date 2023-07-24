#include "invariantgraph/views/intAbsNode.hpp"

#include "views/intAbsView.hpp"

std::unique_ptr<invariantgraph::IntAbsNode>
invariantgraph::IntAbsNode::fromModelConstraint(
    const fznparser::Model&, const fznparser::Constraint& constraint,
    std::unordered_map<std::string_view, VariableNode>& variableMap) {
  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);

  return std::make_unique<invariantgraph::IntAbsNode>(a, b);
}

void invariantgraph::IntAbsNode::createDefinedVariables(Engine& engine) {
  if (definedVariables().front()->varId() == NULL_ID) {
    definedVariables().front()->setVarId(
        engine.makeIntView<IntAbsView>(engine, input()->varId()));
  }
}

void invariantgraph::IntAbsNode::registerWithEngine(Engine&) {}
