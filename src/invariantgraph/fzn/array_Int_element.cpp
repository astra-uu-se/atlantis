#include "invariantgraph/fzn/array_int_element.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_int_element(FznInvariantGraph& invariantGraph,
                       const fznparser::IntArg& idx,
                       std::vector<Int>&& parArray,
                       const fznparser::IntArg& output, Int offset) {
  invariantGraph.addInvariantNode(std::make_unique<ArrayIntElementNode>(
      std::move(parArray), invariantGraph.createVarNode(idx, false),
      invariantGraph.createVarNode(output, true), offset));
  return true;
}

bool array_int_element(FznInvariantGraph& invariantGraph,
                       const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "array_int_element" &&
      constraint.identifier() != "array_int_element_offset") {
    return false;
  }

  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntArg, true);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::IntVarArray, false);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::IntArg, true);

  const fznparser::IntArg& idx =
      std::get<fznparser::IntArg>(constraint.arguments().at(0));

  const Int offset =
      constraint.identifier() != "array_int_element_offset"
          ? 1
          : (idx.isParameter() ? idx.parameter() : idx.var().lowerBound());

  return array_int_element(
      invariantGraph, idx,
      std::move(std::get<fznparser::IntVarArray>(constraint.arguments().at(1))
                    .toParVector()),
      std::get<fznparser::IntArg>(constraint.arguments().at(2)), offset);
}

}  // namespace atlantis::invariantgraph::fzn