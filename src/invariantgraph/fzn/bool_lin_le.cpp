#include "atlantis/invariantgraph/fzn/bool_lin_le.hpp"

#include <numeric>
#include <vector>

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/invariantNodes/boolLinearNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool bool_lin_le(FznInvariantGraph& invariantGraph, std::vector<Int>&& coeffs,
                 const std::shared_ptr<fznparser::BoolVarArray>& inputs,
                 Int bound) {
  const auto [lb, ub] = linBounds(coeffs, inputs);

  const VarNodeId outputVarNodeId = invariantGraph.retrieveIntVarNode(
      SearchDomain(lb, bound), VarNode::DomainType::UPPER_BOUND);

  invariantGraph.addInvariantNode(std::make_unique<BoolLinearNode>(
      std::move(coeffs), invariantGraph.retrieveVarNodes(inputs),
      outputVarNodeId));
  return true;
}

bool bool_lin_le(FznInvariantGraph& invariantGraph,
                 const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "bool_lin_le") {
    return false;
  }
  verifyNumArguments(constraint, 3);
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::IntVarArray, false)
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 1, fznparser::BoolVarArray, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::IntArg, false)

  std::vector<Int> coeffs = std::get<std::shared_ptr<fznparser::IntVarArray>>(
                                constraint.arguments().at(0))
                                ->toParVector();

  return bool_lin_le(
      invariantGraph, std::move(coeffs),
      std::get<std::shared_ptr<fznparser::BoolVarArray>>(
          constraint.arguments().at(1)),
      std::get<fznparser::IntArg>(constraint.arguments().at(2)).toParameter());
}

}  // namespace atlantis::invariantgraph::fzn
