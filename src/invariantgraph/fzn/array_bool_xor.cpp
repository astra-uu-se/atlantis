#include "atlantis/invariantgraph/fzn/array_bool_xor.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/types.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolXorNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_bool_xor(FznInvariantGraph& graph,
                    const std::shared_ptr<fznparser::BoolVarArray>& vars,
                    const fznparser::BoolArg& reified) {
  graph.addInvariantNode(std::make_unique<invariantgraph::ArrayBoolXorNode>(
      graph.retrieveVarNodes(vars), graph.retrieveVarNode(reified)));
  return true;
}

bool array_bool_xor(FznInvariantGraph& graph,
                    const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "array_bool_xor") {
    return false;
  }
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::BoolVarArray, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::BoolArg, true)
  return array_bool_xor(
      graph, getArgArray<fznparser::BoolVarArray>(constraint.arguments().at(0)),
      std::get<fznparser::BoolArg>(constraint.arguments().at(1)));
}

}  // namespace atlantis::invariantgraph::fzn
