#include "atlantis/invariantgraph/fzn/int_ne.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/fzn/int_eq.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intNeNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool int_ne(FznInvariantGraph& invariantGraph, VarNodeId varNodeId, Int value) {
  invariantGraph.varNode(varNodeId).removeValue(value);
  invariantGraph.varNode(varNodeId).tightenDomainType(
      VarNode::DomainType::DOMAIN);
  return true;
}

bool int_ne(FznInvariantGraph& invariantGraph, VarNodeId varNodeId, Int value,
            const fznparser::BoolArg& reified) {
  if (reified.isFixed()) {
    if (reified.toParameter()) {
      return int_ne(invariantGraph, varNodeId, value);
    }
    return int_eq(invariantGraph, varNodeId, value);
  }

  if (!invariantGraph.varNode(varNodeId).inDomain(value)) {
    const VarNodeId reifiedVarNodeId = invariantGraph.retrieveVarNode(reified);
    invariantGraph.varNode(reifiedVarNodeId).fixValue(false);
    return true;
  }
  invariantGraph.addInvariantNode(std::make_unique<IntNeNode>(
      varNodeId, invariantGraph.retrieveVarNode(value),
      invariantGraph.retrieveVarNode(reified)));
  return true;
}

bool int_ne(FznInvariantGraph& invariantGraph, VarNodeId a, VarNodeId b) {
  invariantGraph.addInvariantNode(std::make_unique<IntNeNode>(a, b, true));
  return true;
}

bool int_ne(FznInvariantGraph& invariantGraph, VarNodeId a, VarNodeId b,
            const fznparser::BoolArg& reified) {
  if (reified.isFixed()) {
    if (reified.toParameter()) {
      return int_ne(invariantGraph, a, b);
    }
    return int_eq(invariantGraph, a, b);
  }
  invariantGraph.addInvariantNode(std::make_unique<IntNeNode>(a, b, true));
  return true;
}

bool int_ne(FznInvariantGraph& invariantGraph, VarNodeId varNodeId,
            const fznparser::IntArg& modelArg) {
  if (modelArg.isFixed()) {
    return int_ne(invariantGraph, varNodeId, modelArg.toParameter());
  }
  return int_ne(invariantGraph, varNodeId,
                invariantGraph.retrieveVarNode(modelArg));
}

bool int_ne(FznInvariantGraph& invariantGraph, VarNodeId varNodeId,
            const fznparser::IntArg& modelVar,
            const fznparser::BoolArg& reified) {
  if (modelVar.isFixed()) {
    return int_ne(invariantGraph, varNodeId, modelVar.toParameter(), reified);
  }
  return int_ne(invariantGraph, varNodeId,
                invariantGraph.retrieveVarNode(modelVar), reified);
}

bool int_ne(FznInvariantGraph& invariantGraph, VarNodeId varNodeId,
            const fznparser::IntVar& modelVar) {
  if (modelVar.isFixed()) {
    return int_ne(invariantGraph, varNodeId, modelVar.lowerBound());
  }
  return int_ne(invariantGraph, varNodeId,
                invariantGraph.retrieveVarNode(modelVar));
}

bool int_ne(FznInvariantGraph& invariantGraph, const fznparser::IntArg& a,
            const fznparser::IntArg& b) {
  if (a.isFixed() && b.isFixed()) {
    if (a.toParameter() == b.toParameter()) {
      throw FznArgumentException(
          "int_ne: fixed variables or parameters a and b cannot be equal");
    }
    return true;
  }
  if (a.isFixed()) {
    return int_ne(invariantGraph, invariantGraph.retrieveVarNode(b),
                  a.toParameter());
  }
  if (b.isFixed()) {
    return int_ne(invariantGraph, invariantGraph.retrieveVarNode(a),
                  b.toParameter());
  }
  return int_ne(invariantGraph, invariantGraph.retrieveVarNode(a),
                invariantGraph.retrieveVarNode(b));
}

bool int_ne(FznInvariantGraph& invariantGraph, const fznparser::IntArg& a,
            const fznparser::IntArg& b, const fznparser::BoolArg& reified) {
  if (reified.isFixed()) {
    if (reified.toParameter()) {
      return int_ne(invariantGraph, a, b);
    }
    return int_eq(invariantGraph, a, b);
  }
  if (a.isFixed() && b.isFixed()) {
    const VarNodeId reifiedVarNodeId = invariantGraph.retrieveVarNode(reified);
    invariantGraph.varNode(reifiedVarNodeId)
        .fixValue(a.toParameter() == b.toParameter());
  }
  if (a.isFixed()) {
    return int_ne(invariantGraph, invariantGraph.retrieveVarNode(b),
                  a.toParameter(), reified);
  }
  if (b.isFixed()) {
    return int_ne(invariantGraph, invariantGraph.retrieveVarNode(a),
                  b.toParameter(), reified);
  }
  return int_ne(invariantGraph, invariantGraph.retrieveVarNode(a),
                invariantGraph.retrieveVarNode(b), reified);
}

bool int_ne(FznInvariantGraph& invariantGraph,
            const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "int_ne" &&
      constraint.identifier() != "int_ne_reif") {
    return false;
  }
  const bool isReified = constraintIdentifierIsReified(constraint);
  verifyNumArguments(constraint, isReified ? 3 : 2);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::IntArg, true)

  if (!isReified) {
    return int_ne(invariantGraph,
                  std::get<fznparser::IntArg>(constraint.arguments().at(0)),
                  std::get<fznparser::IntArg>(constraint.arguments().at(1)));
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::BoolArg, true)
  return int_ne(invariantGraph,
                std::get<fznparser::IntArg>(constraint.arguments().at(0)),
                std::get<fznparser::IntArg>(constraint.arguments().at(1)),
                std::get<fznparser::BoolArg>(constraint.arguments().at(2)));
}

}  // namespace atlantis::invariantgraph::fzn
