#include "atlantis/invariantgraph/fzn/bool_clause.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolClauseNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool bool_clause(FznInvariantGraph& invariantGraph,
                 const std::shared_ptr<fznparser::BoolVarArray>& as,
                 const std::shared_ptr<fznparser::BoolVarArray>& bs) {
  invariantGraph.addInvariantNode(
      std::make_unique<invariantgraph::BoolClauseNode>(
          invariantGraph.retrieveVarNodes(as),
          invariantGraph.retrieveVarNodes(bs), true));
  return true;
}

bool bool_clause(FznInvariantGraph& invariantGraph,
                 const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "bool_clause") {
    return false;
  }
  verifyNumArguments(constraint, 2);
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::BoolVarArray, true)
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 1, fznparser::BoolVarArray, true)

  return bool_clause(
      invariantGraph,
      getArgArray<fznparser::BoolVarArray>(constraint.arguments().at(0)),
      getArgArray<fznparser::BoolVarArray>(constraint.arguments().at(1)));
}

}  // namespace atlantis::invariantgraph::fzn
