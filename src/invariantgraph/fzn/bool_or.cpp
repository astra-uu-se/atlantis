

#include "invariantgraph/fzn/bool_or.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"

namespace atlantis::invariantgraph::fzn {

bool bool_or(FznInvariantGraph& invariantGraph, const fznparser::BoolArg& a,
             const fznparser::BoolArg& b, const fznparser::BoolArg& reified) {
  if (reified.isFixed()) {
    invariantGraph.addInvariantNode(std::make_unique<BoolOrNode>(
        invariantGraph.createVarNode(a, false),
        invariantGraph.createVarNode(b, false), reified.toParameter()));
    return true;
  }
  invariantGraph.addInvariantNode(std::make_unique<BoolOrNode>(
      invariantGraph.createVarNode(a, false),
      invariantGraph.createVarNode(b, false),
      invariantGraph.createVarNode(reified.var(), true)));
  return true;
}

bool bool_or(FznInvariantGraph& invariantGraph,
             const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "bool_or") {
    return false;
  }
  verifyNumArguments(constraint, 3);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::BoolArg, true);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::BoolArg, true);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::BoolArg, true);

  return bool_or(invariantGraph,
                 std::get<fznparser::BoolArg>(constraint.arguments().at(0)),
                 std::get<fznparser::BoolArg>(constraint.arguments().at(1)),
                 std::get<fznparser::BoolArg>(constraint.arguments().at(2)));
}

}  // namespace atlantis::invariantgraph::fzn