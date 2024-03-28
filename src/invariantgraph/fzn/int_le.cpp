#include "atlantis/invariantgraph/fzn/int_le.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/fzn/int_lt.hpp"
#include "atlantis/invariantgraph/fzn/int_ne.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intLeNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool int_le(FznInvariantGraph& invariantGraph, VarNodeId a, Int b) {
  invariantGraph.varNode(a).removeValuesAbove(b);
  invariantGraph.varNode(a).tightenDomainType(VarNode::DomainType::UPPER_BOUND);
  return true;
}

bool int_le(FznInvariantGraph& invariantGraph, Int a, VarNodeId b) {
  invariantGraph.varNode(b).removeValuesBelow(a);
  invariantGraph.varNode(b).tightenDomainType(VarNode::DomainType::LOWER_BOUND);
  return true;
}

bool int_le(FznInvariantGraph& invariantGraph, VarNodeId a, Int b,
            const fznparser::BoolArg& reified) {
  if (reified.isFixed()) {
    if (reified.toParameter()) {
      return int_le(invariantGraph, a, b);
    }
    return int_lt(invariantGraph, b, a);
  }

  if (invariantGraph.varNode(a).upperBound() <= b ||
      b < invariantGraph.varNode(a).lowerBound()) {
    const VarNodeId reifiedVarNodeId = invariantGraph.retrieveVarNode(reified);
    invariantGraph.varNode(reifiedVarNodeId)
        .fixValue(invariantGraph.varNode(a).upperBound() <= b);
    return true;
  }
  invariantGraph.addInvariantNode(
      std::make_unique<IntLeNode>(a, invariantGraph.retrieveVarNode(b),
                                  invariantGraph.retrieveVarNode(reified)));
  return true;
}

bool int_le(FznInvariantGraph& invariantGraph, Int a, VarNodeId b,
            const fznparser::BoolArg& reified) {
  if (reified.isFixed()) {
    if (reified.toParameter()) {
      return int_le(invariantGraph, a, b);
    }
    return int_lt(invariantGraph, b, a);
  }

  if (a <= invariantGraph.varNode(b).lowerBound() ||
      a > invariantGraph.varNode(b).upperBound()) {
    // constraint is always satisfied or unsatisfied
    const VarNodeId reifiedVarNodeId = invariantGraph.retrieveVarNode(reified);
    invariantGraph.varNode(reifiedVarNodeId)
        .fixValue(a <= invariantGraph.varNode(b).lowerBound());
    return true;
  }
  invariantGraph.addInvariantNode(
      std::make_unique<IntLeNode>(invariantGraph.retrieveVarNode(a), b,
                                  invariantGraph.retrieveVarNode(reified)));
  return true;
}

bool int_le(FznInvariantGraph& invariantGraph, VarNodeId a, VarNodeId b) {
  invariantGraph.addInvariantNode(std::make_unique<IntLeNode>(a, b, true));
  return true;
}

bool int_le(FznInvariantGraph& invariantGraph, VarNodeId a, VarNodeId b,
            const fznparser::BoolArg& reified) {
  if (reified.isFixed()) {
    if (reified.toParameter()) {
      return int_le(invariantGraph, a, b);
    }
    return int_lt(invariantGraph, b, a);
  }
  invariantGraph.addInvariantNode(std::make_unique<IntLeNode>(a, b, true));
  return true;
}

bool int_le(FznInvariantGraph& invariantGraph, const fznparser::IntArg& a,
            const fznparser::IntArg& b) {
  Int aLb = a.isFixed() ? a.toParameter() : a.var()->lowerBound();
  Int bUb = b.isFixed() ? b.toParameter() : b.var()->upperBound();
  if (aLb > bUb) {
    throw FznArgumentException(
        "int_le: fixed variables or parameters a and b must be equal");
  }
  if (a.isFixed()) {
    return int_le(invariantGraph, invariantGraph.retrieveVarNode(b),
                  a.toParameter());
  }
  if (b.isFixed()) {
    return int_le(invariantGraph, invariantGraph.retrieveVarNode(a),
                  b.toParameter());
  }
  return int_le(invariantGraph, invariantGraph.retrieveVarNode(a),
                invariantGraph.retrieveVarNode(b));
}

bool int_le(FznInvariantGraph& invariantGraph, const fznparser::IntArg& a,
            const fznparser::IntArg& b, const fznparser::BoolArg& reified) {
  if (reified.isFixed()) {
    if (reified.toParameter()) {
      return int_le(invariantGraph, a, b);
    }
    return int_ne(invariantGraph, a, b);
  }
  if (a.isFixed() && b.isFixed()) {
    const VarNodeId reifiedVarNodeId = invariantGraph.retrieveVarNode(reified);
    invariantGraph.varNode(reifiedVarNodeId)
        .fixValue(a.toParameter() == b.toParameter());
  }
  if (a.isFixed()) {
    return int_le(invariantGraph, invariantGraph.retrieveVarNode(b),
                  a.toParameter(), reified);
  }
  if (b.isFixed()) {
    return int_le(invariantGraph, invariantGraph.retrieveVarNode(a),
                  b.toParameter(), reified);
  }
  return int_le(invariantGraph, invariantGraph.retrieveVarNode(a),
                invariantGraph.retrieveVarNode(b), reified);
}

bool int_le(FznInvariantGraph& invariantGraph,
            const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "int_le" &&
      constraint.identifier() != "int_le_reif") {
    return false;
  }
  const bool isReified = constraintIdentifierIsReified(constraint);
  verifyNumArguments(constraint, isReified ? 3 : 2);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::IntArg, true)

  if (!isReified) {
    return int_le(invariantGraph,
                  std::get<fznparser::IntArg>(constraint.arguments().at(0)),
                  std::get<fznparser::IntArg>(constraint.arguments().at(1)));
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::BoolArg, true)
  return int_le(invariantGraph,
                std::get<fznparser::IntArg>(constraint.arguments().at(0)),
                std::get<fznparser::IntArg>(constraint.arguments().at(1)),
                std::get<fznparser::BoolArg>(constraint.arguments().at(2)));
}

}  // namespace atlantis::invariantgraph::fzn
