

#include "invariantgraph/fzn/bool_lt.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "invariantgraph/fzn/bool_le.hpp"

namespace atlantis::invariantgraph::fzn {

bool bool_lt(FznInvariantGraph& invariantGraph, const fznparser::BoolArg& a,
             const fznparser::BoolArg& b) {
  invariantGraph.addInvariantNode(std::make_unique<BoolEqNode>(
      invariantGraph.createVarNodeFromFzn(a, false),
      invariantGraph.createVarNodeFromFzn(b, false), true));
  return true;
}

bool bool_lt(FznInvariantGraph& invariantGraph, const fznparser::BoolArg& a,
             const fznparser::BoolArg& b, const fznparser::BoolArg& reified) {
  if (reified.isFixed()) {
    if (reified.toParameter()) {
      return bool_lt(invariantGraph, a, b);
    }
    return bool_le(invariantGraph, b, a);
  }

  invariantGraph.addInvariantNode(std::make_unique<BoolEqNode>(
      invariantGraph.createVarNodeFromFzn(a, false),
      invariantGraph.createVarNodeFromFzn(b, false),
      invariantGraph.createVarNodeFromFzn(reified.var(), true)));

  return true;
}

bool bool_lt(FznInvariantGraph& invariantGraph,
             const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "bool_lt" &&
      constraint.identifier() != "bool_lt_reif") {
    return false;
  }
  const bool isReified = constraintIdentifierIsReified(constraint);
  verifyNumArguments(constraint, isReified ? 3 : 2);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::BoolArg, true);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::BoolArg, true);

  if (!isReified) {
    return bool_lt(invariantGraph,
                   std::get<fznparser::BoolArg>(constraint.arguments().at(0)),
                   std::get<fznparser::BoolArg>(constraint.arguments().at(1)));
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::BoolArg, true);
  return bool_lt(invariantGraph,
                 std::get<fznparser::BoolArg>(constraint.arguments().at(0)),
                 std::get<fznparser::BoolArg>(constraint.arguments().at(1)),
                 std::get<fznparser::BoolArg>(constraint.arguments().at(2)));
}

}  // namespace atlantis::invariantgraph::fzn