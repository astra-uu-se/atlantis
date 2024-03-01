#include "atlantis/invariantgraph/fzn/int_lin_le.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/fzn/int_le.hpp"
#include "atlantis/invariantgraph/invariantNodes/intLinearNode.hpp"

namespace atlantis::invariantgraph::fzn {

static void verifyInputs(const std::vector<Int>& coeffs,
                         const fznparser::IntVarArray& inputs) {
  if (coeffs.size() != inputs.size()) {
    throw FznArgumentException(
        "int_lin_le constraint first and second array arguments must have the "
        "same length");
  }
}

bool int_lin_le(FznInvariantGraph& invariantGraph, std::vector<Int>&& coeffs,
                const fznparser::IntVarArray& inputs, Int bound) {
  verifyInputs(coeffs, inputs);
  if (coeffs.empty()) {
    if (bound >= 0) {
      return true;
    }
    throw FznArgumentException(
        "int_lin_le constraint: total of empty arrays is always greater than " +
        std::to_string(bound));
  }

  const auto [lb, ub] = linBounds(coeffs, inputs);

  if (lb > bound) {
    throw FznArgumentException(
        "int_lin_le constraint: total is always greater than " +
        std::to_string(bound));
  }
  if (ub <= bound) {
    return true;
  }

  const VarNodeId outputVarNodeId = invariantGraph.retrieveIntVarNode(
      SearchDomain(lb, ub), VarNode::DomainType::UPPER_BOUND);

  invariantGraph.addInvariantNode(std::make_unique<IntLinearNode>(
      std::move(coeffs), invariantGraph.retrieveVarNodes(inputs),
      outputVarNodeId));

  int_le(invariantGraph, outputVarNodeId, bound);

  return true;
}

bool int_lin_le(FznInvariantGraph& invariantGraph, std::vector<Int>&& coeffs,
                const fznparser::IntVarArray& inputs, Int bound,
                const fznparser::BoolArg& reified) {
  verifyInputs(coeffs, inputs);
  if (reified.isFixed()) {
    if (reified.toParameter()) {
      return int_lin_le(invariantGraph, std::move(coeffs), inputs, bound);
    }
    invertCoeffs(coeffs);
    return int_lin_le(invariantGraph, std::move(coeffs), inputs, -bound);
  }

  if (coeffs.empty()) {
    const VarNodeId reifiedVarNodeId = invariantGraph.retrieveVarNode(reified);
    invariantGraph.varNode(reifiedVarNodeId).fixValue(bound >= 0);
    return true;
  }

  const auto& [lb, ub] = linBounds(coeffs, inputs);

  if (ub <= bound) {
    const VarNodeId reifiedVarNodeId = invariantGraph.retrieveVarNode(reified);
    invariantGraph.varNode(reifiedVarNodeId).fixValue(true);
    return true;
  }

  if (bound < lb) {
    const VarNodeId reifiedVarNodeId = invariantGraph.retrieveVarNode(reified);
    invariantGraph.varNode(reifiedVarNodeId).fixValue(false);
    return true;
  }

  const VarNodeId outputVarNodeId = invariantGraph.retrieveIntVarNode(
      SearchDomain(lb, ub), VarNode::DomainType::NONE);

  invariantGraph.addInvariantNode(std::make_unique<IntLinearNode>(
      std::move(coeffs), invariantGraph.retrieveVarNodes(inputs),
      outputVarNodeId));

  int_le(invariantGraph, outputVarNodeId, bound, reified);

  return true;
}

bool int_lin_le(FznInvariantGraph& invariantGraph,
                const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "int_lin_le" &&
      constraint.identifier() != "int_lin_le_reif") {
    return false;
  }
  const bool isReified = constraintIdentifierIsReified(constraint);
  verifyNumArguments(constraint, isReified ? 4 : 3);
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::IntVarArray, false)
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 1, fznparser::IntVarArray, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::IntArg, false)

  std::vector<Int> coeffs =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0))
          .toParVector();

  if (!isReified) {
    return int_lin_le(
        invariantGraph, std::move(coeffs),
        std::get<fznparser::IntVarArray>(constraint.arguments().at(1)),
        std::get<fznparser::IntArg>(constraint.arguments().at(2))
            .toParameter());
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 3, fznparser::BoolArg, true)
  return int_lin_le(
      invariantGraph, std::move(coeffs),
      std::get<fznparser::IntVarArray>(constraint.arguments().at(1)),
      std::get<fznparser::IntArg>(constraint.arguments().at(2)).toParameter(),
      std::get<fznparser::BoolArg>(constraint.arguments().at(3)));
}

}  // namespace atlantis::invariantgraph::fzn
