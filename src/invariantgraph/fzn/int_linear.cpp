#include "invariantgraph/fzn/int_linear.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"

namespace atlantis::invariantgraph::fzn {

bool int_linear(FznInvariantGraph& invariantGraph, std::vector<Int>&& coeffs,
                std::vector<VarNodeId>&& inputVarNodeIds,
                VarNodeId outputVarNodeId, Int sum) {
  if (sum != 0) {
    // The negative sum is the sum of the defined variable:
    coeffs.emplace_back(1);
    inputVarNodeIds.emplace_back(
        invariantGraph.createVarNodeFromFzn(-sum, false));
  }

  invariantGraph.addInvariantNode(std::make_unique<IntLinearNode>(
      std::move(coeffs), std::move(inputVarNodeIds), outputVarNodeId));

  return true;
}

bool int_linear(FznInvariantGraph& invariantGraph, std::vector<Int>&& coeffs,
                const fznparser::IntVarArray& vars,
                const fznparser::Var& definedVar, const Int sum) {
  Int definedVarIndex = -1;
  for (Int i = 0; i < static_cast<Int>(vars.size()); ++i) {
    if (!std::holds_alternative<Int>(vars.at(i)) &&
        std::get<std::reference_wrapper<const fznparser::IntVar>>(vars.at(i))
                .get()
                .identifier() == definedVar.identifier()) {
      definedVarIndex = i;
      break;
    }
  }

  if (definedVarIndex < 0) {
    return false;
  }

  Int outputCoeff = coeffs.at(definedVarIndex);
  // TODO: add a violation that is definedVar % coeffs[definedVarIndex]
  if (std::abs(outputCoeff) != 1) {
    return false;
  }

  coeffs.erase(coeffs.begin() + definedVarIndex);
  if (outputCoeff > 1) {
    std::for_each(coeffs.begin(), coeffs.end(),
                  [](Int& coeff) { coeff *= -1; });
  } else {
    outputCoeff = -outputCoeff;
  }

  std::vector<VarNodeId> inputVarNodeIds;
  inputVarNodeIds.reserve(vars.size());
  VarNodeId outputVarNodeId(NULL_NODE_ID);
  for (Int i = 0; i < static_cast<Int>(vars.size()); ++i) {
    const VarNodeId varNodeId =
        std::holds_alternative<Int>(vars.at(i))
            ? invariantGraph.createVarNodeFromFzn(std::get<Int>(vars.at(i)),
                                                  i == definedVarIndex)
            : invariantGraph.createVarNodeFromFzn(
                  std::get<std::reference_wrapper<const fznparser::IntVar>>(
                      vars.at(i)),
                  i == definedVarIndex);

    if (i == definedVarIndex) {
      outputVarNodeId = varNodeId;
    } else {
      inputVarNodeIds.emplace_back(varNodeId);
    }
  }

  return int_linear(invariantGraph, std::move(coeffs),
                    std::move(inputVarNodeIds), outputVarNodeId, sum);
}

bool int_linear(FznInvariantGraph& invariantGraph,
                const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "int_lin_eq" &&
      constraint.identifier() != "int_lin_eq_reif") {
    return false;
  }
  const bool isReified = constraintIdentifierIsReified(constraint);
  verifyNumArguments(constraint, isReified ? 4 : 3);
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::IntVarArray, false)
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 1, fznparser::IntVarArray, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::IntArg, false)

  if (isReified) {
    FZN_CONSTRAINT_TYPE_CHECK(constraint, 3, fznparser::BoolArg, true)
    const auto& reified =
        std::get<fznparser::BoolArg>(constraint.arguments().at(3));
    if (!reified.isFixed()) {
      return false;
    }
    if (!reified.toParameter()) {
      return false;
    }
  }

  if (!constraint.definedVar().has_value()) {
    return false;
  }

  const fznparser::Var& definedVar = constraint.definedVar().value();

  std::vector<Int> coeffs =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0))
          .toParVector();

  return int_linear(
      invariantGraph, std::move(coeffs),
      std::get<fznparser::IntVarArray>(constraint.arguments().at(1)),
      definedVar,
      std::get<fznparser::IntArg>(constraint.arguments().at(2)).toParameter());
}

}  // namespace atlantis::invariantgraph::fzn