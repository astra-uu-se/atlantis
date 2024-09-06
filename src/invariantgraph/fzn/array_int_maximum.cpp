#include "atlantis/invariantgraph/fzn/array_int_maximum.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayIntMaximumNode.hpp"
#include "atlantis/invariantgraph/types.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_int_maximum(FznInvariantGraph& graph,
                       const fznparser::IntArg& output,
                       const std::shared_ptr<fznparser::IntVarArray>& inputs) {
  graph.addInvariantNode(std::make_shared<ArrayIntMaximumNode>(
      graph, graph.retrieveVarNodes(inputs), graph.retrieveVarNode(output)));
  return true;
}

bool array_int_maximum(FznInvariantGraph& graph,
                       const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "array_int_maximum") {
    return false;
  }
  verifyNumArguments(constraint, 2);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntArg, true)
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 1, fznparser::IntVarArray, true)

  return array_int_maximum(
      graph, std::get<fznparser::IntArg>(constraint.arguments().at(0)),
      getArgArray<fznparser::IntVarArray>(constraint.arguments().at(1)));
}

}  // namespace atlantis::invariantgraph::fzn
