#include "invariantgraph/views/bool2IntNode.hpp"

#include "../parseHelper.hpp"
#include "views/bool2IntView.hpp"

std::unique_ptr<invariantgraph::Bool2IntNode>
invariantgraph::Bool2IntNode::fromModelConstraint(
    const std::shared_ptr<fznparser::Constraint>& constraint,
    const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
        variableMap) {
  MAPPED_SEARCH_VARIABLE_ARG(a, constraint->arguments()[0], variableMap);
  MAPPED_SEARCH_VARIABLE_ARG(b, constraint->arguments()[1], variableMap);

  return std::make_unique<invariantgraph::Bool2IntNode>(a, b);
}

void invariantgraph::Bool2IntNode::registerWithEngine(
    Engine& engine, std::map<VariableNode*, VarId>& variableMap) {
  auto outputVar = engine.makeIntView<::Bool2IntView>(variableMap.at(_input));
  variableMap.emplace(definedVariables()[0], outputVar);
}
