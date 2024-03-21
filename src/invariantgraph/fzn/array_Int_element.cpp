#include "atlantis/invariantgraph/fzn/array_int_element.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayElementNode.hpp"
#include "atlantis/invariantgraph/types.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_int_element(FznInvariantGraph& invariantGraph,
                       const fznparser::IntArg& idx,
                       std::vector<Int>&& parArray,
                       const fznparser::IntArg& output, Int offset) {
  invariantGraph.addInvariantNode(std::make_unique<ArrayElementNode>(
      std::move(parArray), invariantGraph.retrieveVarNode(idx),
      invariantGraph.retrieveVarNode(output), offset));
  return true;
}

bool array_int_element(FznInvariantGraph& invariantGraph,
                       const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "array_int_element" &&
      constraint.identifier() != "array_int_element_offset") {
    return false;
  }

  bool hasOffsetSuffix = hasSuffix(constraint.identifier(), "_offset");
  verifyNumArguments(constraint, hasOffsetSuffix ? 4 : 3);

  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntArg, true)
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 1, fznparser::IntVarArray, false)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::IntArg, true)

  const auto& idx = std::get<fznparser::IntArg>(constraint.arguments().at(0));

  Int offset;
  if (hasOffsetSuffix) {
    FZN_CONSTRAINT_TYPE_CHECK(constraint, 3, fznparser::IntArg, true)
    offset =
        std::get<fznparser::IntArg>(constraint.arguments().at(3)).toParameter();
  } else {
    offset = idx.isParameter() ? idx.parameter() : idx.var()->lowerBound();
  }

  return array_int_element(
      invariantGraph, idx,
      std::get<std::shared_ptr<fznparser::IntVarArray>>(
          constraint.arguments().at(1))
          ->toParVector(),
      std::get<fznparser::IntArg>(constraint.arguments().at(2)), offset);
}

}  // namespace atlantis::invariantgraph::fzn
