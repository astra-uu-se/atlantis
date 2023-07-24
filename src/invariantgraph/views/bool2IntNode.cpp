#include "invariantgraph/views/bool2IntNode.hpp"

#include "views/bool2IntView.hpp"

std::unique_ptr<invariantgraph::Bool2IntNode>
invariantgraph::Bool2IntNode::fromModelConstraint(
    const fznparser::Model&, const fznparser::Constraint& constraint,
    std::unordered_map<std::string_view, VariableNode>& variableMap) {
  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);

  return std::make_unique<invariantgraph::Bool2IntNode>(a, b);
}

void invariantgraph::Bool2IntNode::createDefinedVariables(Engine& engine) {
  if (definedVariables().front()->varId() == NULL_ID) {
    definedVariables().front()->setVarId(
        engine.makeIntView<Bool2IntView>(engine, input()->varId()));
  }
}

void invariantgraph::Bool2IntNode::registerWithEngine(Engine&) {}
