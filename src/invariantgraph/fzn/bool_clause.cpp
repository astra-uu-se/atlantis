

#include "invariantgraph/fzn/bool_clause.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"

namespace atlantis::invariantgraph::fzn {

bool bool_clause(FznInvariantGraph& invariantGraph,
                 const fznparser::BoolVarArray& as,
                 const fznparser::BoolVarArray& bs) {
  invariantGraph.addInvariantNode(
      std::make_unique<invariantgraph::BoolClauseNode>(
          invariantGraph.createVarNodes(as, false),
          invariantGraph.createVarNodes(bs, false), true));
  return true;
}

bool bool_clause(FznInvariantGraph& invariantGraph,
                 const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "bool_clause") {
    return false;
  }
  verifyNumArguments(constraint, 2);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::BoolVarArray, true);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::BoolVarArray, true);

  return bool_clause(
      invariantGraph,
      std::get<fznparser::BoolVarArray>(constraint.arguments().at(0)),
      std::get<fznparser::BoolVarArray>(constraint.arguments().at(1)));
}

}  // namespace atlantis::invariantgraph::fzn