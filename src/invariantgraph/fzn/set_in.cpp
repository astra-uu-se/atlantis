#include "atlantis/invariantgraph/fzn/set_in.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/inIntervalNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/setInNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool set_in(FznInvariantGraph& graph, const fznparser::IntArg& var,
            const fznparser::IntSet& set) {
  if (set.isInterval()) {
    graph.addInvariantNode(std::make_shared<InIntervalNode>(
        graph, graph.retrieveVarNode(var), set.lowerBound(), set.upperBound(),
        true));
  } else {
    graph.addInvariantNode(
        std::make_shared<SetInNode>(graph, graph.retrieveVarNode(var),
                                    std::vector<Int>{set.elements()}, true));
  }
  return true;
}

bool set_in(FznInvariantGraph& graph, const fznparser::IntArg& var,
            const fznparser::IntSet& set, const fznparser::BoolArg& reified) {
  if (set.isInterval()) {
    graph.addInvariantNode(std::make_shared<InIntervalNode>(
        graph, graph.retrieveVarNode(var), set.lowerBound(), set.upperBound(),
        graph.retrieveVarNode(reified)));
  } else {
    graph.addInvariantNode(std::make_shared<SetInNode>(
        graph, graph.retrieveVarNode(var), std::vector<Int>{set.elements()},
        graph.retrieveVarNode(reified)));
  }
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
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::IntSetArg, false)
  if (!isReified) {
    return set_in(graph,
                  std::get<fznparser::IntArg>(constraint.arguments().at(0)),
                  std::get<fznparser::IntSetArg>(constraint.arguments().at(1))
                      .toParameter());
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::BoolArg, true)
  return set_in(graph,
                std::get<fznparser::IntArg>(constraint.arguments().at(0)),
                std::get<fznparser::IntSetArg>(constraint.arguments().at(1))
                    .toParameter(),
                std::get<fznparser::BoolArg>(constraint.arguments().at(2)));
}

}  // namespace atlantis::invariantgraph::fzn
