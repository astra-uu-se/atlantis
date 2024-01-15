

#include "invariantgraph/fzn/int_linear.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"

namespace atlantis::invariantgraph::fzn {

bool int_linear(FznInvariantGraph& invariantGraph,
                const fznparser::Constraint& constraint) {
  // assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  std::vector<Int> coeffs =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0))
          .toParVector();

  const fznparser::IntVarArray& vars =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(1));

  const std::optional<std::reference_wrapper<const fznparser::Var>>
      definedVarRef = constraint.definedVar();

  assert(definedVarRef.has_value());

  const fznparser::IntVar& definedVar =
      std::get<fznparser::IntVar>(definedVarRef.value().get());

  size_t definedVarIndex = vars.size();
  for (size_t i = 0; i < vars.size(); ++i) {
    if (!std::holds_alternative<Int>(vars.at(i)) &&
        std::get<std::reference_wrapper<const fznparser::IntVar>>(vars.at(i))
                .get()
                .identifier() == definedVar.identifier()) {
      definedVarIndex = i;
      break;
    }
  }

  assert(definedVarIndex < vars.size());
  Int outputCoeff = coeffs.at(definedVarIndex);
  // TODO: add a violation that is definedVar % coeffs[definedVarIndex]
  if (std::abs(outputCoeff) != 1) {
    throw std::runtime_error(
        "Cannot define variable with coefficient which is not +/-1");
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
  for (size_t i = 0; i < vars.size(); ++i) {
    if (i == definedVarIndex) {
      continue;
    }
    if (std::holds_alternative<Int>(vars.at(i))) {
      inputVarNodeIds.emplace_back(invariantGraph.createVarNodeFromFzn(
          std::get<Int>(vars.at(i)), false));
    } else {
      inputVarNodeIds.emplace_back(invariantGraph.createVarNodeFromFzn(
          std::get<std::reference_wrapper<const fznparser::IntVar>>(vars.at(i)),
          false));
    }
  }
  auto sum =
      std::get<fznparser::IntArg>(constraint.arguments().at(2)).parameter();
  if (sum != 0) {
    // The negative sum is the offset of the defined variable:
    coeffs.emplace_back(1);
    inputVarNodeIds.emplace_back(
        invariantGraph.createVarNodeFromFzn(-sum, false));
  }

  VarNodeId outputVarNodeId =
      invariantGraph.createVarNodeFromFzn(definedVar, true);

  invariantGraph.addInvariantNode(std::move(std::make_unique<IntLinearNode>(
      std::move(coeffs), std::move(inputVarNodeIds), outputVarNodeId)));
  return true;
}

}  // namespace atlantis::invariantgraph::fzn