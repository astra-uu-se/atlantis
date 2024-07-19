#include "atlantis/invariantgraph/fzn/set_in.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/setInNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool set_in(FznInvariantGraph& graph, const fznparser::IntArg& var,
            const fznparser::IntSetArg& b) {
  std::vector<Int> values = b.toParameter().elements();
  graph.addInvariantNode(std::make_unique<SetInNode>(graph.retrieveVarNode(var),
                                                     std::move(values), true));
  return true;
}

bool set_in(FznInvariantGraph& graph, const fznparser::IntArg& var,
            const fznparser::IntSetArg& b, const fznparser::BoolArg& reified) {
  std::vector<Int> values = b.toParameter().elements();
  graph.addInvariantNode(
      std::make_unique<SetInNode>(graph.retrieveVarNode(var), std::move(values),
                                  graph.retrieveVarNode(reified)));
  return true;
}

bool set_in(FznInvariantGraph& graph, const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "set_in" &&
      constraint.identifier() != "set_in_reif") {
    return false;
  }

  const bool isReified = constraintIdentifierIsReified(constraint);
  verifyNumArguments(constraint, isReified ? 3 : 2);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::IntSetArg, true)
  if (!isReified) {
    return set_in(graph,
                  std::get<fznparser::IntArg>(constraint.arguments().at(0)),
                  std::get<fznparser::IntSetArg>(constraint.arguments().at(1)));
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::BoolArg, true)
  return set_in(graph,
                std::get<fznparser::IntArg>(constraint.arguments().at(0)),
                std::get<fznparser::IntSetArg>(constraint.arguments().at(1)),
                std::get<fznparser::BoolArg>(constraint.arguments().at(2)));
}

}  // namespace atlantis::invariantgraph::fzn
