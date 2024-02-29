

#include "atlantis/invariantgraph/fzn/array_bool_element.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_bool_element(FznInvariantGraph& invariantGraph,
                        const fznparser::IntArg& idx,
                        std::vector<bool>&& parVector,
                        const fznparser::BoolArg& output, const Int offset) {
  invariantGraph.addInvariantNode(std::make_unique<ArrayElementNode>(
      toIntVector(parVector), invariantGraph.retrieveVarNode(idx),
      invariantGraph.retrieveVarNode(output), offset));
  return true;
}

bool array_bool_element(FznInvariantGraph& invariantGraph,
                        const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "array_bool_element" &&
      constraint.identifier() != "array_bool_element_offset") {
    return false;
  }

  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntArg, true)
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 1, fznparser::BoolVarArray, false)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::BoolArg, true)

  const auto& idx = std::get<fznparser::IntArg>(constraint.arguments().at(0));

  const Int offset =
      constraint.identifier() != "array_bool_element_offset"
          ? 1
          : (idx.isParameter() ? idx.parameter() : idx.var().lowerBound());

  return array_bool_element(
      invariantGraph, idx,
      std::get<fznparser::BoolVarArray>(constraint.arguments().at(1))
          .toParVector(),
      std::get<fznparser::BoolArg>(constraint.arguments().at(2)), offset);
}

}  // namespace atlantis::invariantgraph::fzn
