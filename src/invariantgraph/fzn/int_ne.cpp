

#include "invariantgraph/fzn/int_ne.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "invariantgraph/fzn/int_eq.hpp"

namespace atlantis::invariantgraph::fzn {

bool int_ne(FznInvariantGraph& invariantGraph, VarNodeId varNodeId, Int value) {
  invariantGraph.varNode(varNodeId).removeValue(value);
}

bool int_ne(FznInvariantGraph& invariantGraph, VarNodeId varNodeId, Int value,
            const fznparser::BoolArg& reified) {
  if (reified.isFixed()) {
    if (reified.toParameter()) {
      return int_ne(invariantGraph, varNodeId, value);
    }
    return int_eq(invariantGraph, varNodeId, value);
  }

  if (!invariantGraph.varNode(varNodeId).domain().contains(value)) {
    const VarNodeId reifiedVarNodeId =
        invariantGraph.createVarNode(reified, true);
    invariantGraph.varNode(reifiedVarNodeId).fix(0);
    return true;
  }
  invariantGraph.addInvariantNode(std::move(std::make_unique<IntNeNode>(
      varNodeId, value, invariantGraph.createVarNode(reified, true))));
}

bool int_ne(FznInvariantGraph& invariantGraph, VarNodeId a, VarNodeId b) {
  invariantGraph.addInvariantNode(
      std::move(std::make_unique<IntNeNode>(a, b, true)));
}

bool int_ne(FznInvariantGraph& invariantGraph, VarNodeId a, VarNodeId b,
            const fznparser::BoolArg& reified) {
  if (reified.isFixed()) {
    if (reified.toParameter()) {
      return int_ne(invariantGraph, a, b);
    }
    return int_eq(invariantGraph, a, b);
  }
  invariantGraph.addInvariantNode(
      std::move(std::make_unique<IntNeNode>(a, b, true)));
}

bool int_ne(FznInvariantGraph& invariantGraph, VarNodeId varNodeId,
            const fznparser::IntArg& modelArg) {
  if (modelArg.isFixed()) {
    return int_ne(invariantGraph, varNodeId, modelArg.toParameter());
  }
  return int_ne(invariantGraph, varNodeId,
                invariantGraph.createVarNode(modelArg, false));
}

bool int_ne(FznInvariantGraph& invariantGraph, VarNodeId varNodeId,
            const fznparser::IntArg& modelVar,
            const fznparser::BoolArg& reified) {
  if (modelVar.isFixed()) {
    return int_ne(invariantGraph, varNodeId, modelVar.toParameter(), reified);
  }
  return int_ne(invariantGraph, varNodeId,
                invariantGraph.createVarNode(modelVar, false), reified);
}

bool int_ne(FznInvariantGraph& invariantGraph, VarNodeId varNodeId,
            fznparser::IntVar modelVar) {
  if (modelVar.isFixed()) {
    return int_ne(invariantGraph, varNodeId, modelVar.toParameter());
  }
  return int_ne(invariantGraph, varNodeId,
                invariantGraph.createVarNode(modelVar, false));
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
    return int_ne(invariantGraph, invariantGraph.createVarNode(b, false),
                  a.toParameter());
  }
  if (b.isFixed()) {
    return int_ne(invariantGraph, invariantGraph.createVarNode(a, false),
                  b.toParameter());
  }
  return int_ne(invariantGraph, invariantGraph.createVarNode(a, false),
                invariantGraph.createVarNode(b, false));
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
    const reifiedVarNodeId = invariantGraph.createVarNode(reified, true);
    invariantGraph.varNode(reifiedVarNodeId)
        .fix(a.toParameter() == b.toParameter() ? 0 : 1);
  }
  if (a.isFixed()) {
    return int_ne(invariantGraph, invariantGraph.createVarNode(b, false),
                  a.toParameter(), reified);
  }
  if (b.isFixed()) {
    return int_ne(invariantGraph, invariantGraph.createVarNode(a, false),
                  b.toParameter(), reified);
  }
  return int_ne(invariantGraph, invariantGraph.createVarNode(a, false),
                invariantGraph.createVarNode(b, false), reified);
}

bool int_ne(FznInvariantGraph& invariantGraph,
            const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "int_ne" &&
      constraint.identifier() != "int_ne_reif") {
    return false;
  }
  const bool isReified = constraintIdentifierIsReified(constraint);
  verifyNumArguments(constraint, isReified ? 3 : 2);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntArg, true);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::IntArg, true);

  if (!isReified) {
    return int_ne(invariantGraph,
                  std::get<fznparser::IntArg>(constraint.arguments().at(0)),
                  std::get<fznparser::IntArg>(constraint.arguments().at(1)));
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::BoolArg, true);
  return int_ne(invariantGraph,
                std::get<fznparser::IntArg>(constraint.arguments().at(0)),
                std::get<fznparser::IntArg>(constraint.arguments().at(1)),
                std::get<fznparser::BoolArg>(constraint.arguments().at(2)));
}

}  // namespace atlantis::invariantgraph::fzn