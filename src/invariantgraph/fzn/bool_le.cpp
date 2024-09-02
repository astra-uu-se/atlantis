#include "atlantis/invariantgraph/fzn/bool_le.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/fzn/bool_lt.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolLeNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool bool_le(FznInvariantGraph& graph, const fznparser::BoolArg& a,
             const fznparser::BoolArg& b) {
  graph.addInvariantNode(std::make_shared<BoolLeNode>(
      graph, graph.retrieveVarNode(a), graph.retrieveVarNode(b), true));
  return true;
}

bool bool_le(FznInvariantGraph& graph, const fznparser::BoolArg& a,
             const fznparser::BoolArg& b, const fznparser::BoolArg& reified) {
  if (reified.isParameter()) {
    if (reified.toParameter()) {
      return bool_le(graph, a, b);
    } else {
      return bool_lt(graph, b, a);
    }
  }

  graph.addInvariantNode(std::make_shared<BoolLeNode>(
      graph, graph.retrieveVarNode(a), graph.retrieveVarNode(b),
      graph.retrieveVarNode(reified)));

  return true;
}

bool bool_le(FznInvariantGraph& graph,
             const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "bool_le" &&
      constraint.identifier() != "bool_le_reif") {
    return false;
  }
  const bool isReified = constraintIdentifierIsReified(constraint);
  verifyNumArguments(constraint, isReified ? 3 : 2);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::BoolArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::BoolArg, true)

  if (!isReified) {
    return bool_le(graph,
                   std::get<fznparser::BoolArg>(constraint.arguments().at(0)),
                   std::get<fznparser::BoolArg>(constraint.arguments().at(1)));
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::BoolArg, true)
  return bool_le(graph,
                 std::get<fznparser::BoolArg>(constraint.arguments().at(0)),
                 std::get<fznparser::BoolArg>(constraint.arguments().at(1)),
                 std::get<fznparser::BoolArg>(constraint.arguments().at(2)));
}

}  // namespace atlantis::invariantgraph::fzn
