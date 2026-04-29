#include "atlantis/invariantgraph/fzn/int_min.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayIntMinimumNode.hpp"
#include "atlantis/invariantgraph/varNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool int_min(FznInvariantGraph& graph, const fznparser::IntArg& a,
             const fznparser::IntArg& b, const fznparser::IntArg& minimum) {
  graph.addInvariantNode(std::make_shared<ArrayIntMinimumNode>(
      graph, graph.retrieveVarNode(a), graph.retrieveVarNode(b),
      graph.retrieveVarNode(minimum)));
  return true;
}

bool int_min(FznInvariantGraph& graph,
             const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "int_min") {
    return false;
  }
  verifyNumArguments(constraint, 3);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntArg, true);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::IntArg, true);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::IntArg, true);

  return int_min(graph,
                 std::get<fznparser::IntArg>(constraint.arguments().at(0)),
                 std::get<fznparser::IntArg>(constraint.arguments().at(1)),
                 std::get<fznparser::IntArg>(constraint.arguments().at(2)));
}

}  // namespace atlantis::invariantgraph::fzn
