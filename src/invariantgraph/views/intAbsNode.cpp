#include "invariantgraph/views/intAbsNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::IntAbsNode>
invariantgraph::IntAbsNode::fromModelConstraint(
    const std::shared_ptr<fznparser::Constraint>& constraint,
    const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
        variableMap) {
  MAPPED_SEARCH_VARIABLE_ARG(a, constraint->arguments()[0], variableMap);
  SEARCH_VARIABLE_ARG(b, constraint->arguments()[1]);

  return std::make_unique<invariantgraph::IntAbsNode>(a, b);
}
