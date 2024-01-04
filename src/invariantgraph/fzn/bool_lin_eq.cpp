

#include "invariantgraph/fzn/bool_lin_eq.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"

namespace atlantis::invariantgraph::fzn {

bool bool_lin_eq(FznInvariantGraph& invariantGraph, std::vector<Int>&& coeffs,
                 const fznparser::BoolVarArray inputs, Int bound) {
  if (coeffs.size() != inputs.size()) {
    throw FznArgumentException(
        "bool_lin_eq constraint first and second array arguments must have the "
        "same length");
  }
  if (coeffs.empty()) {
    if (bound == 0) {
      return true;
    }
    throw FznArgumentException(
        "bool_lin_eq constraint with empty arrays must have a total sum of 0");
  }

  const VarNodeId outputVarNodeId =
      invariantGraph.createVarNode(SearchDomain(bound, bound), true, true);

  invariantGraph.varNode(outputVarNodeId).shouldEnforceDomain(true);

  invariantGraph.addInvariantNode(std::move(std::make_unique<BoolLinearNode>(
      std::move(coeffs),
      std::move(invariantGraph.createVarNodes(inputs, false)),
      outputVarNodeId)));

  return true;
}

bool bool_lin_eq(FznInvariantGraph& invariantGraph,
                 const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "bool_lin_eq") {
    return false;
  }
  verifyNumArguments(constraint, 3);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntVarArray, false);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::BoolVarArray, true);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::IntArg, false);

  std::vector<Int> coeffs =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0))
          .toParVector();

  return bool_lin_eq(
      invariantGraph, std::move(coeffs),
      std::get<fznparser::BoolVarArray>(constraint.arguments().at(1)),
      std::get<fznparser::IntArg>(constraint.arguments().at(2)).toParameter());
}

}  // namespace atlantis::invariantgraph::fzn