

#include "invariantgraph/fzn/set_in.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"

namespace atlantis::invariantgraph::fzn {

bool set_in(FznInvariantGraph& invariantGraph, const fznparser::IntArg& a,
            const fznparser::IntSetArg& b) {
  std::vector<Int> values = b.toParameter().elements();
  invariantGraph.addInvariantNode(std::make_unique<SetInNode>(
      invariantGraph.createVarNode(a, false), std::move(values), true));
  return true;
}

bool set_in(FznInvariantGraph& invariantGraph,
            const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "set_in") {
    return false;
  }

  verifyNumArguments(constraint, isReified ? 2);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntArg, true);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::IntSetArg, true);

  return set_in(invariantGraph,
                std::get<fznparser::IntArg>(constraint.arguments().at(0)),
                std::get<fznparser::IntSetArg>(constraint.arguments().at(1)));
}

}  // namespace atlantis::invariantgraph::fzn