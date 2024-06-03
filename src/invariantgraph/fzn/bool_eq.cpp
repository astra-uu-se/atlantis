#include "atlantis/invariantgraph/fzn/bool_eq.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/fzn/bool_xor.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolEqNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool bool_eq(FznInvariantGraph& invariantGraph, VarNodeId varNodeId,
             bool value) {
  invariantGraph.varNode(varNodeId).fixToValue(value);
  invariantGraph.varNode(varNodeId).setDomainType(VarNode::DomainType::FIXED);
  return true;
}

bool bool_eq(FznInvariantGraph& invariantGraph, VarNodeId a, VarNodeId b) {
  invariantGraph.addInvariantNode(std::make_unique<BoolEqNode>(a, b, true));
  return true;
}

bool bool_eq(FznInvariantGraph& invariantGraph, const fznparser::BoolArg& a,
             const fznparser::BoolArg& b) {
  return bool_eq(invariantGraph, invariantGraph.retrieveVarNode(a),
                 invariantGraph.retrieveVarNode(b));
}

bool bool_eq(FznInvariantGraph& invariantGraph, VarNodeId a, VarNodeId b,
             VarNodeId reifiedVarNodeId) {
  const VarNode& reifiedVarNode = invariantGraph.varNode(reifiedVarNodeId);
  if (reifiedVarNode.isFixed()) {
    if (reifiedVarNode.lowerBound() == 0) {
      return bool_eq(invariantGraph, a, b);
    }
    // (a xor b) == (x != b)
    return bool_xor(invariantGraph, a, b);
  }

  invariantGraph.addInvariantNode(
      std::make_unique<BoolEqNode>(a, b, reifiedVarNodeId));

  return true;
}

bool bool_eq(FznInvariantGraph& invariantGraph, const fznparser::BoolArg& a,
             const fznparser::BoolArg& b, const fznparser::BoolArg& reified) {
  if (reified.isFixed()) {
    if (reified.toParameter()) {
      return bool_eq(invariantGraph, a, b);
    }
    // (a xor b) == (x != b)
    return bool_xor(invariantGraph, a, b);
  }

  invariantGraph.addInvariantNode(std::make_unique<BoolEqNode>(
      invariantGraph.retrieveVarNode(a), invariantGraph.retrieveVarNode(b),
      invariantGraph.retrieveVarNode(reified.var())));

  return true;
}

bool bool_eq(FznInvariantGraph& invariantGraph,
             const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "bool_and" &&
      constraint.identifier() != "bool_and_reif") {
    return false;
  }
  const bool isReified = constraintIdentifierIsReified(constraint);
  verifyNumArguments(constraint, isReified ? 3 : 2);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::BoolArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::BoolArg, true)

  if (!isReified) {
    return bool_eq(invariantGraph,
                   std::get<fznparser::BoolArg>(constraint.arguments().at(0)),
                   std::get<fznparser::BoolArg>(constraint.arguments().at(1)));
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::BoolArg, true)
  return bool_eq(invariantGraph,
                 std::get<fznparser::BoolArg>(constraint.arguments().at(0)),
                 std::get<fznparser::BoolArg>(constraint.arguments().at(1)),
                 std::get<fznparser::BoolArg>(constraint.arguments().at(2)));
}

}  // namespace atlantis::invariantgraph::fzn
