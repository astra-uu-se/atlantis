#include "invariantgraph/views/intLinLeReifNode.hpp"

#include "../parseHelper.hpp"
#include "invariantgraph/constraints/intLinLeNode.hpp"
#include "invariantgraph/views/reifiedConstraint.hpp"

std::unique_ptr<invariantgraph::IntLinLeReifNode>
invariantgraph::IntLinLeReifNode::fromModelConstraint(
    const std::shared_ptr<fznparser::Constraint> &constraint,
    const std::function<VariableNode *(std::shared_ptr<fznparser::Variable>)>
        &variableMap) {
  assert(constraint->name() == "int_lin_le_reif");
  assert(constraint->arguments().size() == 4);

  VALUE_VECTOR_ARG(as, constraint->arguments()[0]);
  MAPPED_SEARCH_VARIABLE_VECTOR_ARG(bs, constraint->arguments()[1], variableMap);
  VALUE_ARG(c, constraint->arguments()[2]);
  MAPPED_SEARCH_VARIABLE_ARG(r, constraint->arguments()[3], variableMap);

  return std::make_unique<invariantgraph::IntLinLeReifNode>(
      std::make_unique<invariantgraph::IntLinLeNode>(as, bs, c), r);
}
