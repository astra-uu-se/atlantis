#include "invariantgraph/views/intLinNeReifNode.hpp"

#include "../parseHelper.hpp"
#include "invariantgraph/constraints/intLinNeNode.hpp"
#include "invariantgraph/views/reifiedConstraint.hpp"

std::unique_ptr<invariantgraph::IntLinNeReifNode>
invariantgraph::IntLinNeReifNode::fromModelConstraint(
    const std::shared_ptr<fznparser::Constraint> &constraint,
    const std::function<VariableNode *(std::shared_ptr<fznparser::Variable>)>
        &variableMap) {
  assert(constraint->name() == "int_lin_ne_reif");
  assert(constraint->arguments().size() == 4);

  VALUE_VECTOR_ARG(as, constraint->arguments()[0]);
  MAPPED_SEARCH_VARIABLE_VECTOR_ARG(bs, constraint->arguments()[1], variableMap);
  VALUE_ARG(c, constraint->arguments()[2]);
  SEARCH_VARIABLE_ARG(r, constraint->arguments()[3]);

  return std::make_unique<invariantgraph::IntLinNeReifNode>(
      std::make_unique<invariantgraph::IntLinNeNode>(as, bs, c), r);
}
