#include "invariantgraph/views/boolNotNode.hpp"

std::unique_ptr<invariantgraph::BoolNotNode>
invariantgraph::BoolNotNode::fromModelConstraint(
    const fznparser::Model&, const fznparser::Constraint& constraint,
    std::unordered_map<std::string_view, VariableNode>& variableMap) {
  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);

  return std::make_unique<invariantgraph::BoolNotNode>(a, b);
}

void invariantgraph::BoolNotNode::createDefinedVariables(Engine& engine) {
  if (definedVariables().front()->varId() == NULL_ID) {
    definedVariables().front()->setVarId(
        engine.makeIntView<Bool2IntView>(engine, input()->varId()));
  }
}

void invariantgraph::BoolNotNode::registerWithEngine(Engine&) {}
