#include "atlantis/invariantgraph/fzn/int_lin_ne.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/invariantNodes/intLinearNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/allDifferentNode.hpp"

namespace atlantis::invariantgraph::fzn {

static void verifyInputs(
    const std::vector<Int>& coeffs,
    const std::shared_ptr<fznparser::IntVarArray>& inputs) {
  if (coeffs.size() != inputs->size()) {
    throw FznArgumentException(
        "int_lin_ne constraint first and second array arguments must have the "
        "same length");
  }
}

bool int_lin_ne(FznInvariantGraph& invariantGraph, std::vector<Int>&& coeffs,
                const std::shared_ptr<fznparser::IntVarArray>& inputs,
                Int bound) {
  verifyInputs(coeffs, inputs);
  if (coeffs.empty()) {
    if (bound != 0) {
      return true;
    }
    throw FznArgumentException(
        "int_lin_ne constraint with empty arrays must have a total sum other "
        "than 0");
  }

  const auto& [lb, ub] = linBounds(coeffs, inputs);

  if (bound < lb || ub < bound) {
    // always holds:
    return true;
  }

  SearchDomain dom(lb, ub);
  dom.remove(bound);

  const VarNode::DomainType dt =
      (lb == bound ? VarNode::DomainType::LOWER_BOUND
                   : (ub == bound ? VarNode::DomainType::UPPER_BOUND
                                  : VarNode::DomainType::DOMAIN));

  const VarNodeId outputVarNodeId =
      invariantGraph.retrieveIntVarNode(std::move(dom), dt);

  invariantGraph.addInvariantNode(std::make_unique<IntLinearNode>(
      std::move(coeffs), invariantGraph.retrieveVarNodes(inputs),
      outputVarNodeId));

  return true;
}

bool int_lin_ne(FznInvariantGraph& invariantGraph, std::vector<Int>&& coeffs,
                const std::shared_ptr<fznparser::IntVarArray>& inputs,
                Int bound, const fznparser::BoolArg& reified) {
  verifyInputs(coeffs, inputs);

  const auto& [lb, ub] = linBounds(coeffs, inputs);

  const VarNodeId outputVarNodeId = invariantGraph.retrieveIntVarNode(
      SearchDomain(lb, ub), VarNode::DomainType::NONE);

  invariantGraph.addInvariantNode(std::make_unique<IntLinearNode>(
      std::move(coeffs), invariantGraph.retrieveVarNodes(inputs),
      outputVarNodeId));

  invariantGraph.addInvariantNode(std::make_unique<AllDifferentNode>(
      outputVarNodeId, invariantGraph.retrieveIntVarNode(bound),
      invariantGraph.retrieveVarNode(reified)));

  return true;
}

bool int_lin_ne(FznInvariantGraph& invariantGraph,
                const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "int_lin_ne" &&
      constraint.identifier() != "int_lin_ne_reif") {
    return false;
  }
  const bool isReified = constraintIdentifierIsReified(constraint);
  verifyNumArguments(constraint, isReified ? 4 : 3);
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::IntVarArray, false)
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 1, fznparser::IntVarArray, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::IntArg, false)

  std::vector<Int> coeffs =
      getArgArray<fznparser::IntVarArray>(constraint.arguments().at(0))
          ->toParVector();

  if (!isReified) {
    return int_lin_ne(
        invariantGraph, std::move(coeffs),
        getArgArray<fznparser::IntVarArray>(constraint.arguments().at(1)),
        std::get<fznparser::IntArg>(constraint.arguments().at(2))
            .toParameter());
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 3, fznparser::BoolArg, true)
  return int_lin_ne(
      invariantGraph, std::move(coeffs),
      getArgArray<fznparser::IntVarArray>(constraint.arguments().at(1)),
      std::get<fznparser::IntArg>(constraint.arguments().at(2)).toParameter(),
      std::get<fznparser::BoolArg>(constraint.arguments().at(3)));
}

}  // namespace atlantis::invariantgraph::fzn
