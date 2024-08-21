#include "atlantis/invariantgraph/fzn/bool_lt.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/fzn/bool_le.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolLtNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool bool_lt(FznInvariantGraph& graph, const fznparser::BoolArg& a,
             const fznparser::BoolArg& b) {
  graph.addInvariantNode(std::make_unique<BoolLtNode>(
      graph.retrieveVarNode(a), graph.retrieveVarNode(b), true));
  return true;
}

bool bool_lt(FznInvariantGraph& graph, const fznparser::BoolArg& a,
             const fznparser::BoolArg& b, const fznparser::BoolArg& reified) {
  graph.addInvariantNode(std::make_unique<BoolLtNode>(
      graph.retrieveVarNode(a), graph.retrieveVarNode(b),
      graph.retrieveVarNode(reified)));

  return true;
}

bool bool_lt(FznInvariantGraph& graph,
             const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "bool_lt" &&
      constraint.identifier() != "bool_lt_reif") {
    return false;
  }
  const bool isReified = constraintIdentifierIsReified(constraint);
  verifyNumArguments(constraint, isReified ? 3 : 2);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::BoolArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::BoolArg, true)

  if (!isReified) {
    return bool_lt(graph,
                   std::get<fznparser::BoolArg>(constraint.arguments().at(0)),
                   std::get<fznparser::BoolArg>(constraint.arguments().at(1)));
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::BoolArg, true)
  return bool_lt(graph,
                 std::get<fznparser::BoolArg>(constraint.arguments().at(0)),
                 std::get<fznparser::BoolArg>(constraint.arguments().at(1)),
                 std::get<fznparser::BoolArg>(constraint.arguments().at(2)));
}

}  // namespace atlantis::invariantgraph::fzn
