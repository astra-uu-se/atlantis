#include "invariantgraph/views/setInReifNode.hpp"

#include "../parseHelper.hpp"
#include "invariantgraph/constraints/setInNode.hpp"
#include "invariantgraph/views/reifiedConstraint.hpp"

std::unique_ptr<invariantgraph::SetInReifNode>
invariantgraph::SetInReifNode::fromModelConstraint(
    const std::shared_ptr<fznparser::Constraint> &constraint,
    const std::function<VariableNode *(std::shared_ptr<fznparser::Variable>)>
        &variableMap) {
  assert(constraint->name() == "set_in_reif");
  assert(constraint->arguments().size() == 3);

  MAPPED_SEARCH_VARIABLE_ARG(a, constraint->arguments()[0], variableMap);
  VALUE_VECTOR_ARG(values, constraint->arguments()[1]);
  MAPPED_SEARCH_VARIABLE_ARG(r, constraint->arguments()[2], variableMap);

  return std::make_unique<invariantgraph::SetInReifNode>(
      std::make_unique<invariantgraph::SetInNode>(a, values), r);
}
