

#include "invariantgraph/fzn/bool_and.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"

namespace atlantis::invariantgraph::fzn {

bool bool_and(FznInvariantGraph& invariantGraph, const fznparser::BoolArg& a,
              const fznparser::BoolArg& b, const fznparser::BoolArg& reified) {
  if (reified.isFixed()) {
    invariantGraph.addInvariantNode(std::make_unique<BoolAndNode>(
        invariantGraph.inputVarNode(a), invariantGraph.inputVarNode(b),
        reified.toParameter()));
    return true;
  }
  invariantGraph.addInvariantNode(std::make_unique<BoolAndNode>(
      invariantGraph.inputVarNode(a), invariantGraph.inputVarNode(b),
      invariantGraph.defineVarNode(reified.var())));
  return true;
}

bool bool_and(FznInvariantGraph& invariantGraph,
              const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "bool_and") {
    return false;
  }
  verifyNumArguments(constraint, 3);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::BoolArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::BoolArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::BoolArg, true)

  return bool_and(invariantGraph,
                  std::get<fznparser::BoolArg>(constraint.arguments().at(0)),
                  std::get<fznparser::BoolArg>(constraint.arguments().at(1)),
                  std::get<fznparser::BoolArg>(constraint.arguments().at(2)));
}

}  // namespace atlantis::invariantgraph::fzn