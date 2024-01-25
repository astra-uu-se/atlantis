#include "invariantgraph/fzn/int_lt.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "invariantgraph/fzn/int_le.hpp"

namespace atlantis::invariantgraph::fzn {

bool int_lt(FznInvariantGraph& invariantGraph, VarNodeId a, Int b) {
  invariantGraph.varNode(a).removeValuesAbove(b - 1);
  invariantGraph.varNode(a).shouldEnforceDomain(true);
  return true;
}

bool int_lt(FznInvariantGraph& invariantGraph, Int a, VarNodeId b) {
  invariantGraph.varNode(b).removeValuesBelow(a + 1);
  invariantGraph.varNode(b).shouldEnforceDomain(true);
  return true;
}

bool int_lt(FznInvariantGraph& invariantGraph, VarNodeId a, Int b,
            const fznparser::BoolArg& reified) {
  if (reified.isFixed()) {
    if (reified.toParameter()) {
      return int_lt(invariantGraph, a, b);
    }
    return int_le(invariantGraph, b, a);
  }

  if (invariantGraph.varNode(a).domain().upperBound() < b ||
      b >= invariantGraph.varNode(a).domain().lowerBound()) {
    const VarNodeId reifiedVarNodeId =
        invariantGraph.createVarNodeFromFzn(reified, true);
    invariantGraph.varNode(reifiedVarNodeId)
        .fixValue(invariantGraph.varNode(a).domain().upperBound() < b);
    return true;
  }
  invariantGraph.addInvariantNode(std::move(std::make_unique<IntLtNode>(
      a, invariantGraph.createVarNodeFromFzn(b, false),
      invariantGraph.createVarNodeFromFzn(reified, true))));
  return true;
}

bool int_lt(FznInvariantGraph& invariantGraph, Int a, VarNodeId b,
            const fznparser::BoolArg& reified) {
  if (reified.isFixed()) {
    if (reified.toParameter()) {
      return int_lt(invariantGraph, a, b);
    }
    return int_lt(invariantGraph, b, a);
  }

  if (a <= invariantGraph.varNode(b).domain().lowerBound() ||
      a > invariantGraph.varNode(b).domain().upperBound()) {
    // constraint is always satisfied or unsatisfied
    const VarNodeId reifiedVarNodeId =
        invariantGraph.createVarNodeFromFzn(reified, true);
    invariantGraph.varNode(reifiedVarNodeId)
        .fixValue(a <= invariantGraph.varNode(b).domain().lowerBound());
    return true;
  }
  invariantGraph.addInvariantNode(std::move(std::make_unique<IntLtNode>(
      invariantGraph.createVarNodeFromFzn(a, false), b,
      invariantGraph.createVarNodeFromFzn(reified, true))));
  return true;
}

bool int_lt(FznInvariantGraph& invariantGraph, VarNodeId a, VarNodeId b) {
  invariantGraph.addInvariantNode(
      std::move(std::make_unique<IntLtNode>(a, b, true)));
  return true;
}

bool int_lt(FznInvariantGraph& invariantGraph, VarNodeId a, VarNodeId b,
            const fznparser::BoolArg& reified) {
  if (reified.isFixed()) {
    if (reified.toParameter()) {
      return int_lt(invariantGraph, a, b);
    }
    return int_lt(invariantGraph, b, a);
  }
  invariantGraph.addInvariantNode(
      std::move(std::make_unique<IntLtNode>(a, b, true)));
  return true;
}

bool int_lt(FznInvariantGraph& invariantGraph, const fznparser::IntArg& a,
            const fznparser::IntArg& b) {
  Int aLb = a.isFixed() ? a.toParameter() : a.var().lowerBound();
  Int bUb = b.isFixed() ? b.toParameter() : b.var().upperBound();
  if (aLb > bUb) {
    throw FznArgumentException(
        "int_lt: fixed variables or parameters a and b must be equal");
    return true;
  }
  if (a.isFixed()) {
    return int_lt(invariantGraph, invariantGraph.createVarNodeFromFzn(b, false),
                  a.toParameter());
  }
  if (b.isFixed()) {
    return int_lt(invariantGraph, invariantGraph.createVarNodeFromFzn(a, false),
                  b.toParameter());
  }
  return int_lt(invariantGraph, invariantGraph.createVarNodeFromFzn(a, false),
                invariantGraph.createVarNodeFromFzn(b, false));
}

bool int_lt(FznInvariantGraph& invariantGraph, const fznparser::IntArg& a,
            const fznparser::IntArg& b, const fznparser::BoolArg& reified) {
  if (reified.isFixed()) {
    if (reified.toParameter()) {
      return int_lt(invariantGraph, a, b);
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
    return int_lt(invariantGraph, invariantGraph.createVarNodeFromFzn(b, false),
                  a.toParameter(), reified);
  }
  if (b.isFixed()) {
    return int_lt(invariantGraph, invariantGraph.createVarNodeFromFzn(a, false),
                  b.toParameter(), reified);
  }
  return int_lt(invariantGraph, invariantGraph.createVarNodeFromFzn(a, false),
                invariantGraph.createVarNodeFromFzn(b, false), reified);
}

bool int_lt(FznInvariantGraph& invariantGraph,
            const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "int_lt" &&
      constraint.identifier() != "int_lt_reif") {
    return false;
  }
  const bool isReified = constraintIdentifierIsReified(constraint);
  verifyNumArguments(constraint, isReified ? 3 : 2);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntArg, true);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::IntArg, true);

  if (!isReified) {
    return int_lt(invariantGraph,
                  std::get<fznparser::IntArg>(constraint.arguments().at(0)),
                  std::get<fznparser::IntArg>(constraint.arguments().at(1)));
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::BoolArg, true);
  return int_lt(invariantGraph,
                std::get<fznparser::IntArg>(constraint.arguments().at(0)),
                std::get<fznparser::IntArg>(constraint.arguments().at(1)),
                std::get<fznparser::BoolArg>(constraint.arguments().at(2)));
}

}  // namespace atlantis::invariantgraph::fzn