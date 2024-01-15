

#include "invariantgraph/fzn/int_plus.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"

namespace atlantis::invariantgraph::fzn {

bool int_plus(FznInvariantGraph& invariantGraph, const fznparser::IntArg a,
              const fznparser::IntArg b, const fznparser::IntArg sum) {
  invariantGraph.addInvariantNode(std::move(std::make_unique<IntPlusNode>(
      invariantGraph.createVarNodeFromFzn(a, false),
      invariantGraph.createVarNodeFromFzn(b, false),
      invariantGraph.createVarNodeFromFzn(sum, true))));
  return true;
  return true;
}

bool int_plus(FznInvariantGraph& invariantGraph,
              const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "int_plus") {
    return false;
  }
  verifyNumArguments(constraint, 3);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntArg, true);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::IntArg, true);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 3, fznparser::IntArg, true);

  return int_plus(invariantGraph,
                  std::get<fznparser::IntArg>(constraint.arguments().at(0)),
                  std::get<fznparser::IntArg>(constraint.arguments().at(1)),
                  std::get<fznparser::IntArg>(constraint.arguments().at(2)));
}

}  // namespace atlantis::invariantgraph::fzn