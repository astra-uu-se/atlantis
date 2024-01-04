



#include "./fznHelper.hpp"
#include "../parseHelper.hpp"


#include "invariantgraph/fzn/int_linear.hpp"

namespace atlantis::invariantgraph::fzn {

static bool int_linear(
    FznInvariantGraph& invariantGraph, const fznparser::Constraint& constraint) {
  // assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  std::vector<Int> coeffs =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0))
          .toParVector();

  const fznparser::IntVarArray& vars =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(1));

  // The negative sum is the offset of the defined variable:
  auto sum =
      std::get<fznparser::IntArg>(constraint.arguments().at(2)).parameter();

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

  Int definedVarCoeff = coeffs.at(definedVarIndex);

  // TODO: add a violation that is definedVar % coeffs[definedVarIndex]
  if (std::abs(definedVarCoeff) != 1) {
    throw std::runtime_error(
        "Cannot define variable with coefficient which is not +/-1");
  }

  auto coeffsIt = coeffs.begin();
  std::advance(coeffsIt, definedVarIndex);
  coeffs.erase(coeffsIt);

  std::vector<VarNodeId> addedVars = invariantGraph.createVarNodes(vars);
  auto varsIt = addedVars.begin();
  std::advance(varsIt, definedVarIndex);
  auto output = *varsIt;
  addedVars.erase(varsIt);

  return std::make_unique<IntLinearNode>(
      std::move(coeffs), std::move(addedVars), output, definedVarCoeff, sum);
}

}  // namespace atlantis::invariantgraph::fzn