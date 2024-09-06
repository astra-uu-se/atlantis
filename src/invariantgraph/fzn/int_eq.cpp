#include "atlantis/invariantgraph/fzn/int_eq.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intAllEqualNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool int_eq(FznInvariantGraph& graph, VarNodeId a, VarNodeId b) {
  graph.addInvariantNode(std::make_shared<IntAllEqualNode>(graph, a, b, false));
  return true;
}

bool int_eq(FznInvariantGraph& graph, const fznparser::IntArg& a,
            const fznparser::IntArg& b) {
  graph.addInvariantNode(std::make_shared<IntAllEqualNode>(
      graph, graph.retrieveVarNode(a), graph.retrieveVarNode(b)));
  return true;
}

bool int_eq(FznInvariantGraph& graph, const fznparser::IntArg& a,
            const fznparser::IntArg& b, const fznparser::BoolArg& reified) {
  graph.addInvariantNode(std::make_shared<IntAllEqualNode>(
      graph, graph.retrieveVarNode(a), graph.retrieveVarNode(b),
      graph.retrieveVarNode(reified)));
  return true;
}

bool int_eq(FznInvariantGraph& graph, const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "int_eq" &&
      constraint.identifier() != "int_eq_reif") {
    return false;
  }
  const bool isReified = constraintIdentifierIsReified(constraint);
  verifyNumArguments(constraint, isReified ? 3 : 2);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::IntArg, true)

  if (!isReified) {
    return int_eq(graph,
                  std::get<fznparser::IntArg>(constraint.arguments().at(0)),
                  std::get<fznparser::IntArg>(constraint.arguments().at(1)));
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::BoolArg, true)
  return int_eq(graph,
                std::get<fznparser::IntArg>(constraint.arguments().at(0)),
                std::get<fznparser::IntArg>(constraint.arguments().at(1)),
                std::get<fznparser::BoolArg>(constraint.arguments().at(2)));
}

}  // namespace atlantis::invariantgraph::fzn
