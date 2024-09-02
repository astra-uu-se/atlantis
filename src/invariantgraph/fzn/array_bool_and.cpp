#include "atlantis/invariantgraph/fzn/array_bool_and.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolAndNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_bool_and(
    FznInvariantGraph& graph,
    const std::shared_ptr<fznparser::BoolVarArray>& boolVarArray,
    const fznparser::BoolArg& reified) {
  graph.addInvariantNode(std::make_shared<ArrayBoolAndNode>(
      graph, graph.retrieveVarNodes(boolVarArray),
      graph.retrieveVarNode(reified)));
  return true;
}

bool array_bool_and(FznInvariantGraph& graph,
                    const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "array_bool_and") {
    return false;
  }
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::BoolVarArray, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::BoolArg, true)

  return array_bool_and(
      graph, getArgArray<fznparser::BoolVarArray>(constraint.arguments().at(0)),
      std::get<fznparser::BoolArg>(constraint.arguments().at(1)));
}

}  // namespace atlantis::invariantgraph::fzn
