

#include "invariantgraph/fzn/int_lin_eq.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "invariantgraph/fzn/int_lin_ne.hpp"

namespace atlantis::invariantgraph::fzn {

void verifyInputs(const std::vector<Int>& coeffs,
                  const fznparser::IntVarArray inputs) {
  if (coeffs.size() != inputs.size()) {
    throw FznArgumentException(
        "int_lin_eq constraint first and second array arguments must have the "
        "same length");
  }
}

bool int_lin_eq(FznInvariantGraph& invariantGraph, std::vector<Int>&& coeffs,
                const fznparser::IntVarArray& inputs, Int bound) {
  verifyInputs(coeffs, inputs);
  if (coeffs.empty()) {
    if (bound == 0) {
      return true;
    }
    throw FznArgumentException(
        "int_lin_eq constraint with empty arrays must have a total sum of 0");
  }

  const auto& [lb, ub] = linBounds(coeffs, inputs);

  const VarNodeId outputVarNodeId =
      invariantGraph.createVarNode(SearchDomain(lb, ub), true, true);

  invariantGraph.addInvariantNode(std::move(std::make_unique<IntLinearNode>(
      std::move(coeffs),
      std::move(invariantGraph.createVarNodes(inputs, false)),
      outputVarNodeId)));

  int_eq(invariantGraph, outputVarNodeId, bound);

  return true;
}

bool int_lin_eq(FznInvariantGraph& invariantGraph, std::vector<Int>&& coeffs,
                const fznparser::IntVarArray& inputs, Int bound,
                fznparser::BoolArg reified) {
  verifyInputs(coeffs, inputs);
  if (reified.isFixed()) {
    if (reified.toParameter()) {
      return int_lin_eq(invariantGraph, std::move(coeffs), inputs, bound);
    }
    return int_lin_ne(invariantGraph, std::move(coeffs), inputs, bound);
  }
  if (coeffs.empty()) {
    const VarNodeId reifiedVarNodeId =
        invariantGraph.createVarNodeFromFzn(reified, true);
    invariantGraph.varNode(reifiedVarNodeId).fixValue(bound == 0);
    return true;
  }

  const auto& [lb, ub] = linBounds(coeffs, inputs);

  const VarNodeId outputVarNodeId =
      invariantGraph.createVarNode(SearchDomain(lb, ub), true, true);

  invariantGraph.addInvariantNode(std::move(std::make_unique<IntLinearNode>(
      std::move(coeffs),
      std::move(invariantGraph.createVarNodes(inputs, false)),
      outputVarNodeId)));

  int_eq(invariantGraph, outputVarNodeId, bound);

  return true;
}

bool int_lin_eq(FznInvariantGraph& invariantGraph,
                const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "int_lin_eq" &&
      constraint.identifier() != "int_lin_eq_reif") {
    return false;
  }
  const bool isReified = constraintIdentifierIsReified(constraint);
  verifyNumArguments(constraint, isReified ? 4 : 3);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntVarArray, false);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::IntVarArray, true);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::IntArg, false);

  std::vector<Int> coeffs =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0))
          .toParVector();

  if (!isReified) {
    return int_lin_eq(
        invariantGraph, std::move(coeffs),
        std::get<fznparser::IntVarArray>(constraint.arguments().at(1)),
        std::get<fznparser::IntArg>(constraint.arguments().at(2))
            .toParameter());
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 3, fznparser::BoolArg, true);
  return int_lin_eq(
      invariantGraph, std::move(coeffs),
      std::get<fznparser::IntVarArray>(constraint.arguments().at(1)),
      std::get<fznparser::IntArg>(constraint.arguments().at(2)).toParameter(),
      std::get<fznparser::BoolArg>(constraint.arguments().at(3)));
}

}  // namespace atlantis::invariantgraph::fzn