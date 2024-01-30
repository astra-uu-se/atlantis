#include "invariantgraph/fzn/int_eq.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "invariantgraph/fzn/int_ne.hpp"

namespace atlantis::invariantgraph::fzn {

bool int_eq(FznInvariantGraph& invariantGraph, VarNodeId varNodeId, Int value) {
  invariantGraph.varNode(varNodeId).fixValue(value);
  invariantGraph.varNode(varNodeId).shouldEnforceDomain(true);
  return true;
}

bool int_eq(FznInvariantGraph& invariantGraph, VarNodeId varNodeId, Int value,
            const fznparser::BoolArg& reified) {
  if (reified.isFixed()) {
    if (reified.toParameter()) {
      return int_eq(invariantGraph, varNodeId, value);
    }
    return int_ne(invariantGraph, varNodeId, value);
  }

  if (!invariantGraph.varNode(varNodeId).inDomain(value)) {
    const VarNodeId reifiedVarNodeId =
        invariantGraph.createVarNodeFromFzn(reified, true);
    invariantGraph.varNode(reifiedVarNodeId).fixValue(false);
    return true;
  }
  invariantGraph.addInvariantNode(std::make_unique<IntEqNode>(
      varNodeId, invariantGraph.createVarNode(value, false), invariantGraph.createVarNodeFromFzn(reified, true)));
  return true;
}

bool int_eq(FznInvariantGraph& invariantGraph, VarNodeId a, VarNodeId b) {
  invariantGraph.addInvariantNode(
      std::make_unique<IntEqNode>(a, b, false));
  return true;
}

bool int_eq(FznInvariantGraph& invariantGraph, VarNodeId a, VarNodeId b,
            VarNodeId reifiedVarNodeId) {
  const VarNode& reifiedVarNode = invariantGraph.varNode(reifiedVarNodeId);
  if (reifiedVarNode.isFixed()) {
    if (reifiedVarNode.lowerBound() == 0) {
      return int_eq(invariantGraph, a, b);
    }
    return int_ne(invariantGraph, a, b);
  }
  invariantGraph.addInvariantNode(
      std::make_unique<IntEqNode>(a, b, reifiedVarNodeId));
  return true;
}

bool int_eq(FznInvariantGraph& invariantGraph, VarNodeId a, VarNodeId b,
            const fznparser::BoolArg& reified) {
  if (reified.isFixed()) {
    if (reified.toParameter()) {
      return int_eq(invariantGraph, a, b);
    }
    return int_ne(invariantGraph, a, b);
  }
  invariantGraph.addInvariantNode(std::make_unique<IntEqNode>(
      a, b, invariantGraph.createVarNodeFromFzn(reified.var(), true)));
  return true;
}

bool int_eq(FznInvariantGraph& invariantGraph, const fznparser::IntArg& a,
            const fznparser::IntArg& b) {
  if (a.isFixed() && b.isFixed()) {
    if (a.toParameter() != b.toParameter()) {
      throw FznArgumentException(
          "int_eq: fixed variables or parameters a and b must be equal");
    }
    return true;
  }
  if (a.isFixed()) {
    return int_eq(invariantGraph, invariantGraph.createVarNodeFromFzn(b, false),
                  a.toParameter());
  }
  if (b.isFixed()) {
    return int_eq(invariantGraph, invariantGraph.createVarNodeFromFzn(a, false),
                  b.toParameter());
  }
  return int_eq(invariantGraph, invariantGraph.createVarNodeFromFzn(a, false),
                invariantGraph.createVarNodeFromFzn(b, false));
}

bool int_eq(FznInvariantGraph& invariantGraph, const fznparser::IntArg& a,
            const fznparser::IntArg& b, const fznparser::BoolArg& reified) {
  if (reified.isFixed()) {
    if (reified.toParameter()) {
      return int_eq(invariantGraph, a, b);
    }
    return int_ne(invariantGraph, a, b);
  }
  if (a.isFixed() && b.isFixed()) {
    const VarNodeId reifiedVarNodeId =
        invariantGraph.createVarNodeFromFzn(reified, true);
    invariantGraph.varNode(reifiedVarNodeId)
        .fixValue(a.toParameter() == b.toParameter());
    invariantGraph.varNode(reifiedVarNodeId).shouldEnforceDomain(true);
  }
  if (a.isFixed()) {
    return int_eq(invariantGraph, invariantGraph.createVarNodeFromFzn(b, false),
                  a.toParameter(), reified);
  }
  if (b.isFixed()) {
    return int_eq(invariantGraph, invariantGraph.createVarNodeFromFzn(a, false),
                  b.toParameter(), reified);
  }
  return int_eq(invariantGraph, invariantGraph.createVarNodeFromFzn(a, false),
                invariantGraph.createVarNodeFromFzn(b, false), reified);
}

bool int_eq(FznInvariantGraph& invariantGraph,
            const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "int_eq" &&
      constraint.identifier() != "int_eq_reif") {
    return false;
  }
  const bool isReified = constraintIdentifierIsReified(constraint);
  verifyNumArguments(constraint, isReified ? 3 : 2);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::IntArg, true)

  if (!isReified) {
    return int_eq(invariantGraph,
                  std::get<fznparser::IntArg>(constraint.arguments().at(0)),
                  std::get<fznparser::IntArg>(constraint.arguments().at(1)));
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::BoolArg, true)
  return int_eq(invariantGraph,
                std::get<fznparser::IntArg>(constraint.arguments().at(0)),
                std::get<fznparser::IntArg>(constraint.arguments().at(1)),
                std::get<fznparser::BoolArg>(constraint.arguments().at(2)));
}

}  // namespace atlantis::invariantgraph::fzn