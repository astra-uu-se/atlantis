#include "invariantgraph/views/intAbsNode.hpp"

#include "../parseHelper.hpp"
#include "views/intAbsView.hpp"

std::unique_ptr<invariantgraph::IntAbsNode>
invariantgraph::IntAbsNode::fromModelConstraint(
    const std::shared_ptr<fznparser::Constraint>& constraint,
    const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
        variableMap) {
  MAPPED_SEARCH_VARIABLE_ARG(a, constraint->arguments()[0], variableMap);
  MAPPED_SEARCH_VARIABLE_ARG(b, constraint->arguments()[1], variableMap);

  return std::make_unique<invariantgraph::IntAbsNode>(a, b);
}

void invariantgraph::IntAbsNode::registerWithEngine(
    Engine& engine, std::map<VariableNode*, VarId>& variableMap) {

  auto outputVar = engine.makeIntView<::IntAbsView>(variableMap.at(_input));
  variableMap.emplace(definedVariables()[0], outputVar);
}
