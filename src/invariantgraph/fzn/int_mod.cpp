

#include "invariantgraph/fzn/int_mod.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"

namespace atlantis::invariantgraph::fzn {

bool int_mod(FznInvariantGraph& invariantGraph,
             const fznparser::IntArg nominator,
             const fznparser::IntArg denominator,
             const fznparser::IntArg remainder) {
  invariantGraph.addInvariantNode(
      std::move(std::make_unique<IntModNode>(
          invariantGraph.createVarNode(nominator, false),
          invariantGraph.createVarNode(denominator, false))),
      invariantGraph.createVarNode(remainder, true));
  return true;
  return true;
}

bool int_mod(FznInvariantGraph& invariantGraph,
             const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "int_mod") {
    return false;
  }
  verifyNumArguments(constraint, 3);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntArg, true);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::IntArg, true);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 3, fznparser::IntArg, true);

  return int_mod(invariantGraph,
                 std::get<fznparser::IntArg>(constraint.arguments().at(0)),
                 std::get<fznparser::IntArg>(constraint.arguments().at(1)),
                 std::get<fznparser::IntArg>(constraint.arguments().at(2)));
}

}  // namespace atlantis::invariantgraph::fzn