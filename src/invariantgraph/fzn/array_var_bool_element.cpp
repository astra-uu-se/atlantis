

#include "invariantgraph/fzn/array_var_bool_element.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_var_bool_element(FznInvariantGraph& invariantGraph,
                            const fznparser::IntArg& index,
                            const fznparser::BoolVarArray& inputs,
                            const fznparser::BoolArg& output, Int offset) {
  invariantGraph.addInvariantNode(std::make_unique<ArrayVarBoolElementNode>(
      invariantGraph.createVarNode(index, false),
      invariantGraph.createVarNodes(inputs, false),
      invariantGraph.createVarNode(output, true), offset));
  return true;
}

bool array_var_bool_element(FznInvariantGraph& invariantGraph,
                            const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "array_var_bool_element" &&
      constraint.identifier() != "array_var_bool_element_nonshifted") {
    return false;
  }

  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntArg, true);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::BoolVarArray, true);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::BoolArg, true);

  const fznparser::IntArg& index =
      std::get<fznparser::IntArg>(constraint.arguments().at(0));

  // Compute offset if nonshifted variant:
  Int offset = constraint.identifier() != "array_var_bool_element_nonshifted"
                   ? 1
                   : (index.isParameter() ? index.parameter()
                                          : index.var().lowerBound());

  return array_var_bool_element(
      invariantGraph, index,
      std::get<fznparser::BoolVarArray>(constraint.arguments().at(1)),
      std::get<fznparser::BoolArg>(constraint.arguments().at(2)), offset);
}

}  // namespace atlantis::invariantgraph::fzn