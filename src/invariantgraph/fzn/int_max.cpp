

#include "invariantgraph/fzn/int_max.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"

namespace atlantis::invariantgraph::fzn {

bool int_max(FznInvariantGraph& invariantGraph, const fznparser::IntArg a,
             const fznparser::IntArg b, const fznparser::IntArg maximum) {
  invariantGraph.addInvariantNode(std::move(std::make_unique<IntMaxNode>(
                                      invariantGraph.createVarNode(a, false),
                                      invariantGraph.createVarNode(b, false))),
                                  invariantGraph.createVarNode(maximum, true));
  return true;
}

bool int_max(FznInvariantGraph& invariantGraph,
             const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "int_max") {
    return false;
  }
  verifyNumArguments(constraint, 3);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntArg, true);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::IntArg, true);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 3, fznparser::IntArg, true);

  return int_max(invariantGraph,
                 std::get<fznparser::IntArg>(constraint.arguments().at(0)),
                 std::get<fznparser::IntArg>(constraint.arguments().at(1)),
                 std::get<fznparser::IntArg>(constraint.arguments().at(2)));
}

}  // namespace atlantis::invariantgraph::fzn