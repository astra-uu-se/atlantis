#include "atlantis/invariantgraph/fzn/set_in.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/setInNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool set_in(FznInvariantGraph& invariantGraph, VarNodeId varNodeId,
            std::vector<Int>&& values, VarNodeId reified) {
  invariantGraph.addInvariantNode(
      std::make_unique<SetInNode>(varNodeId, std::move(values), reified));
  return true;
}

bool set_in(FznInvariantGraph& invariantGraph, const fznparser::IntArg& var,
            const fznparser::IntSetArg& b) {
  std::vector<Int> values = b.toParameter().elements();
  invariantGraph.addInvariantNode(std::make_unique<SetInNode>(
      invariantGraph.retrieveVarNode(var), std::move(values), true));
  return true;
}

bool set_in(FznInvariantGraph& invariantGraph,
            const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "set_in") {
    return false;
  }

  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::IntSetArg, true)

  return set_in(invariantGraph,
                std::get<fznparser::IntArg>(constraint.arguments().at(0)),
                std::get<fznparser::IntSetArg>(constraint.arguments().at(1)));
}

}  // namespace atlantis::invariantgraph::fzn
