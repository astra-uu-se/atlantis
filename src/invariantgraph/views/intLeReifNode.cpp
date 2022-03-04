#include "invariantgraph/views/intLeReifNode.hpp"

#include "../parseHelper.hpp"
#include "invariantgraph/constraints/intEqNode.hpp"
#include "invariantgraph/views/reifiedConstraint.hpp"

std::unique_ptr<invariantgraph::IntLeReifNode>
invariantgraph::IntLeReifNode::fromModelConstraint(
    const std::shared_ptr<fznparser::Constraint> &constraint,
    const std::function<VariableNode *(std::shared_ptr<fznparser::Variable>)>
        &variableMap) {
  assert(constraint->name() == "int_le_reif");
  assert(constraint->arguments().size() == 3);

  MAPPED_SEARCH_VARIABLE_ARG(a, constraint->arguments()[0], variableMap);
  MAPPED_SEARCH_VARIABLE_ARG(b, constraint->arguments()[1], variableMap);
  SEARCH_VARIABLE_ARG(r, constraint->arguments()[2]);

  return std::make_unique<invariantgraph::IntLeReifNode>(
      std::make_unique<invariantgraph::IntEqNode>(a, b), r);
}
