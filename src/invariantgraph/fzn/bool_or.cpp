#include "atlantis/invariantgraph/fzn/bool_or.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolOrNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool bool_or(FznInvariantGraph& graph, const fznparser::BoolArg& a,
             const fznparser::BoolArg& b, const fznparser::BoolArg& reified) {
  graph.addInvariantNode(std::make_unique<ArrayBoolOrNode>(
      graph.retrieveVarNode(a), graph.retrieveVarNode(b),
      graph.retrieveVarNode(reified)));
  return true;
}

bool bool_or(FznInvariantGraph& graph,
             const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "bool_or") {
    return false;
  }
  verifyNumArguments(constraint, 3);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::BoolArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::BoolArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::BoolArg, true)

  return bool_or(graph,
                 std::get<fznparser::BoolArg>(constraint.arguments().at(0)),
                 std::get<fznparser::BoolArg>(constraint.arguments().at(1)),
                 std::get<fznparser::BoolArg>(constraint.arguments().at(2)));
}

}  // namespace atlantis::invariantgraph::fzn
