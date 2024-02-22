

#include "invariantgraph/fzn/bool_lin_eq.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"

namespace atlantis::invariantgraph::fzn {

bool bool_lin_eq(FznInvariantGraph& invariantGraph, std::vector<Int>&& coeffs,
                 std::vector<VarNodeId>&& inputVarNodeIds, Int sum) {
  if (coeffs.size() != inputVarNodeIds.size()) {
    throw FznArgumentException(
        "bool_lin_eq constraint first and second array arguments must have the "
        "same length");
  }
  if (coeffs.empty()) {
    if (sum != 0) {
      throw FznArgumentException(
          "bool_lin_eq constraint with empty arrays must have a total sum of "
          "0");
    }
    return true;
  }

  const VarNodeId outputVarNodeId = invariantGraph.defineIntVarNode();

  invariantGraph.addInvariantNode(std::make_unique<BoolLinearNode>(
      std::move(coeffs), std::move(inputVarNodeIds), outputVarNodeId));

  return true;
}

bool bool_lin_eq(FznInvariantGraph& invariantGraph, std::vector<Int>&& coeffs,
                 std::vector<VarNodeId>&& inputVarNodeIds,
                 VarNodeId outputVarNodeId) {
  if (coeffs.size() != inputVarNodeIds.size()) {
    throw FznArgumentException(
        "bool_lin_eq constraint first and second array arguments must have the "
        "same length");
  }
  if (coeffs.empty()) {
    if (!invariantGraph.varNode(outputVarNodeId).inDomain(Int(0))) {
      throw FznArgumentException(
          "bool_lin_eq constraint with empty arrays must have a total sum of "
          "0");
    }
    invariantGraph.varNode(outputVarNodeId).fixValue(Int(0));
    return true;
  }

  invariantGraph.addInvariantNode(std::make_unique<BoolLinearNode>(
      std::move(coeffs), std::move(inputVarNodeIds), outputVarNodeId));

  return true;
}

bool bool_lin_eq(FznInvariantGraph& invariantGraph, std::vector<Int>&& coeffs,
                 const fznparser::BoolVarArray& inputs,
                 const fznparser::IntArg& outputVar) {
  if (outputVar.isFixed()) {
    return bool_lin_eq(invariantGraph, std::move(coeffs),
                       invariantGraph.inputVarNodes(inputs),
                       outputVar.toParameter());
  }
  return bool_lin_eq(invariantGraph, std::move(coeffs),
                     invariantGraph.inputVarNodes(inputs),
                     invariantGraph.defineVarNode(outputVar));
}

bool bool_lin_eq(FznInvariantGraph& invariantGraph,
                 const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "bool_lin_eq") {
    return false;
  }
  verifyNumArguments(constraint, 3);
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::IntVarArray, false)
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 1, fznparser::BoolVarArray, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::IntArg, false)

  std::vector<Int> coeffs =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0))
          .toParVector();

  return bool_lin_eq(
      invariantGraph, std::move(coeffs),
      std::get<fznparser::BoolVarArray>(constraint.arguments().at(1)),
      std::get<fznparser::IntArg>(constraint.arguments().at(2)).toParameter());
}

}  // namespace atlantis::invariantgraph::fzn