#include "atlantis/invariantgraph/fzn/int_plus.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/invariantNodes/intPlusNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool int_plus(FznInvariantGraph& graph, const fznparser::IntArg& a,
              const fznparser::IntArg& b, const fznparser::IntArg& sum) {
  graph.addInvariantNode(std::make_unique<IntPlusNode>(
      graph.retrieveVarNode(a), graph.retrieveVarNode(b),
      graph.retrieveVarNode(sum)));
  return true;
}

bool int_plus(FznInvariantGraph& graph,
              const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "int_plus") {
    return false;
  }
  verifyNumArguments(constraint, 3);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::IntArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 3, fznparser::IntArg, true)

  return int_plus(graph,
                  std::get<fznparser::IntArg>(constraint.arguments().at(0)),
                  std::get<fznparser::IntArg>(constraint.arguments().at(1)),
                  std::get<fznparser::IntArg>(constraint.arguments().at(2)));
}

}  // namespace atlantis::invariantgraph::fzn
