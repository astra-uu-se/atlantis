

#include "invariantgraph/fzn/array_var_int_element.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_var_int_element(FznInvariantGraph& invariantGraph,
                           const fznparser::IntArg index,
                           const fznparser::IntVarArray& inputs,
                           const fznparser::IntArg& output, Int offset) {
  invariantGraph.addInvariantNode(std::make_unique<ArrayVarElementNode>(
      invariantGraph.createVarNodeFromFzn(index, false),
      std::move(invariantGraph.createVarNodes(inputs, false)),
      invariantGraph.createVarNodeFromFzn(output, true), offset));
  return true;
}

bool array_var_int_element(FznInvariantGraph& invariantGraph,
                           const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "array_var_int_element" &&
      constraint.identifier() != "array_var_int_element_nonshifted") {
    return false;
  }

  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntArg, true);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::IntVarArray, true);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::IntArg, true);

  const fznparser::IntArg& index =
      std::get<fznparser::IntArg>(constraint.arguments().at(0));

  // Compute offset if nonshifted variant:
  Int offset = constraint.identifier() != "array_var_bool_element_nonshifted"
                   ? 1
                   : (index.isParameter() ? index.parameter()
                                          : index.var().lowerBound());

  return array_var_int_element(
      invariantGraph, index,
      std::get<fznparser::IntVarArray>(constraint.arguments().at(1)),
      std::get<fznparser::IntArg>(constraint.arguments().at(2)), offset);
}

}  // namespace atlantis::invariantgraph::fzn