#include "atlantis/invariantgraph/fzn/bool_clause.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/fznInvariantGraph.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolClauseNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool bool_clause(FznInvariantGraph& graph,
                 const std::shared_ptr<fznparser::BoolVarArray>& as,
                 const std::shared_ptr<fznparser::BoolVarArray>& bs) {
  graph.addInvariantNode(std::make_shared<BoolClauseNode>(
      graph, graph.retrieveVarNodes(as), graph.retrieveVarNodes(bs), true));
  return true;
}

bool bool_clause_reif(FznInvariantGraph& graph,
                 const std::shared_ptr<fznparser::BoolVarArray>& as,
                 const std::shared_ptr<fznparser::BoolVarArray>& bs,
                 const fznparser::BoolArg& reified) {
  graph.addInvariantNode(std::make_shared<BoolClauseNode>(
      graph, graph.retrieveVarNodes(as), graph.retrieveVarNodes(bs), graph.retrieveVarNode(reified)));

  return true;
}

bool bool_clause(FznInvariantGraph& graph,
                 const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "bool_clause" &&
      constraint.identifier() != "bool_clause_reif") {
    return false;
  }
  const bool isReified = constraintIdentifierIsReified(constraint);
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::BoolVarArray, true)
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 1, fznparser::BoolVarArray, true)
  if (!isReified) {
    return bool_clause(
      graph, getArgArray<fznparser::BoolVarArray>(constraint.arguments().at(0)),
      getArgArray<fznparser::BoolVarArray>(constraint.arguments().at(1)));
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::BoolArg, true)
  return bool_clause_reif(
      graph, getArgArray<fznparser::BoolVarArray>(constraint.arguments().at(0)),
      getArgArray<fznparser::BoolVarArray>(constraint.arguments().at(1)),
        get<fznparser::BoolArg>(constraint.arguments().at(2)));
}

}  // namespace atlantis::invariantgraph::fzn
