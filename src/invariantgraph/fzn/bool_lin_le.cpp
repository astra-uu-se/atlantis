

#include "invariantgraph/fzn/bool_lin_le.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"

namespace atlantis::invariantgraph::fzn {

bool bool_lin_le(FznInvariantGraph& invariantGraph, std::vector<Int>&& coeffs,
                 const fznparser::BoolVarArray inputs, Int bound) {
  if (coeffs.size() != inputs.size()) {
    throw FznArgumentException(
        "bool_lin_le constraint first and second array arguments must have the "
        "same length");
  }
  if (coeffs.empty()) {
    if (bound == 0) {
      return true;
    }
    throw FznArgumentException(
        "bool_lin_le constraint with empty arrays must have a total sum of 0");
  }

  Int lb = 0;
  for (const Int c : coeffs) {
    if (c < 0) {
      lb += c;
    }
  }

  if (lb > bound) {
    throw FznArgumentException(
        "bool_lin_le constraint, no subset can be less than " +
        std::to_string(bound) + ".");
  }

  const VarNodeId outputVarNodeId =
      invariantGraph.createVarNode(SearchDomain(lb, bound), true, true);

  invariantGraph.varNode(outputVarNodeId).shouldEnforceDomain(true);

  std::make_unique<BoolLinearNode>(std::move(coeffs),
                                   invariantGraph.createVarNodes(inputs, false),
                                   outputVarNodeId);

  return true;
}

bool bool_lin_le(FznInvariantGraph& invariantGraph,
                 const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "bool_lin_le") {
    return false;
  }
  verifyNumArguments(constraint, 3);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntVarArray, false);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::BoolVarArray, true);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::IntArg, false);

  std::vector<Int> coeffs =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0))
          .toParVector();

  return bool_lin_le(
      invariantGraph, std::move(coeffs),
      std::get<fznparser::BoolVarArray>(constraint.arguments().at(1)),
      std::get<fznparser::IntArg>(constraint.arguments().at(2)).toParameter());
}

}  // namespace atlantis::invariantgraph::fzn