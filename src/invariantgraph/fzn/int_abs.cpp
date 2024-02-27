#include "invariantgraph/fzn/int_abs.hpp"

#include "../parseHelper.hpp"
#include "fznHelper.hpp"

namespace atlantis::invariantgraph::fzn {

bool int_abs(FznInvariantGraph& invariantGraph, const fznparser::IntArg& var,
             const fznparser::IntArg& absVar) {
  invariantGraph.addInvariantNode(
      std::make_unique<IntAbsNode>(invariantGraph.retrieveVarNode(var),
                                   invariantGraph.retrieveVarNode(absVar)));
  return true;
}

bool int_abs(FznInvariantGraph& invariantGraph,
             const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "int_abs") {
    return false;
  }
  verifyNumArguments(constraint, 2);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::IntArg, true)

  return int_abs(invariantGraph,
                 std::get<fznparser::IntArg>(constraint.arguments().at(0)),
                 std::get<fznparser::IntArg>(constraint.arguments().at(1)));
}

}  // namespace atlantis::invariantgraph::fzn