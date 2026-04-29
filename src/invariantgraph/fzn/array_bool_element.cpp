#include "atlantis/invariantgraph/fzn/array_bool_element.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayElementNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_bool_element(FznInvariantGraph& graph, const fznparser::IntArg& idx,
                        std::vector<bool>&& parVector,
                        const fznparser::BoolArg& output, const Int offset) {
  graph.addInvariantNode(std::make_shared<ArrayElementNode>(
      graph, std::move(parVector), graph.retrieveVarNode(idx),
      graph.retrieveVarNode(output), offset));
  return true;
}

bool array_bool_element(FznInvariantGraph& graph,
                        const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "array_bool_element" &&
      constraint.identifier() != "array_bool_element_offset") {
    return false;
  }

  const bool hasOffsetSuffix = hasSuffix(constraint.identifier(), "_offset");
  verifyNumArguments(constraint, hasOffsetSuffix ? 4 : 3);

  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntArg, true);
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 1, fznparser::BoolVarArray, false);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::BoolArg, true);

  const auto& idx = std::get<fznparser::IntArg>(constraint.arguments().at(0));

  Int offset;
  if (hasOffsetSuffix) {
    FZN_CONSTRAINT_TYPE_CHECK(constraint, 3, fznparser::IntArg, true);
    offset =
        std::get<fznparser::IntArg>(constraint.arguments().at(3)).toParameter();
  } else {
    offset = idx.isParameter() ? idx.parameter() : idx.var()->lowerBound();
  }

  return array_bool_element(
      graph, idx,
      getArgArray<fznparser::BoolVarArray>(constraint.arguments().at(1))
          ->toParVector(),
      std::get<fznparser::BoolArg>(constraint.arguments().at(2)), offset);
}

}  // namespace atlantis::invariantgraph::fzn
