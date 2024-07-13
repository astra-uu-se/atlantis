#include "atlantis/invariantgraph/fzn/array_var_int_element.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayVarElementNode.hpp"
#include "atlantis/invariantgraph/types.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_var_int_element(
    FznInvariantGraph& invariantGraph, const fznparser::IntArg& index,
    const std::shared_ptr<fznparser::IntVarArray>& inputs,
    const fznparser::IntArg& output, Int offset) {
  invariantGraph.addInvariantNode(std::make_unique<ArrayVarElementNode>(
      invariantGraph.retrieveVarNode(index),
      invariantGraph.retrieveVarNodes(inputs),
      invariantGraph.retrieveVarNode(output), offset));
  return true;
}

bool array_var_int_element(FznInvariantGraph& invariantGraph,
                           const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "array_var_int_element" &&
      constraint.identifier() != "array_var_int_element_offset" &&
      constraint.identifier() != "array_var_int_element_nonshifted") {
    return false;
  }

  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntArg, true)
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 1, fznparser::IntVarArray, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::IntArg, true)
  const auto& index = std::get<fznparser::IntArg>(constraint.arguments().at(0));
  Int offset = 1;
  if (constraint.identifier() != "array_var_int_element_nonshifted") {
    FZN_CONSTRAINT_TYPE_CHECK(constraint, 3, fznparser::IntArg, false)
    offset =
        std::get<fznparser::IntArg>(constraint.arguments().at(3)).toParameter();
  } else {
    // Compute offset if nonshifted variant:
    offset =
        index.isParameter() ? index.parameter() : index.var()->lowerBound();
  }

  return array_var_int_element(
      invariantGraph, index,
      getArgArray<fznparser::IntVarArray>(constraint.arguments().at(1)),
      std::get<fznparser::IntArg>(constraint.arguments().at(2)), offset);
}

}  // namespace atlantis::invariantgraph::fzn
