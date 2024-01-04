

#include "invariantgraph/fzn/array_int_minimum.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_int_minimum(FznInvariantGraph& invariantGraph,
                       const fznparser::IntArg& output,
                       const fznparser::IntVarArray& inputs) {
  invariantGraph.addInvariantNode(std::make_unique<ArrayIntMinimumNode>(
      invariantGraph.createVarNodes(inputs, false),
      invariantGraph.createVarNode(output, true)));
  return true;
}

bool array_int_minimum(FznInvariantGraph& invariantGraph,
                       const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "array_int_minimum") {
    return false;
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntArg, true);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntVarArray, true);

  return array_int_minimum(
      invariantGraph, std::get<fznparser::IntArg>(constraint.arguments().at(0)),
      std::get<fznparser::IntVarArray>(constraint.arguments().at(1)));
}

}  // namespace atlantis::invariantgraph::fzn