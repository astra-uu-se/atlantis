#include "atlantis/invariantgraph/fzn/fzn_global_cardinality_low_up_closed.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/globalCardinalityLowUpClosedNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool fzn_global_cardinality_low_up_closed(
    FznInvariantGraph& invariantGraph,
    const std::shared_ptr<fznparser::IntVarArray>& inputs,
    std::vector<Int>&& cover, std::vector<Int>&& low, std::vector<Int>&& up) {
  invariantGraph.addInvariantNode(
      std::make_unique<GlobalCardinalityLowUpClosedNode>(
          invariantGraph.retrieveVarNodes(inputs), std::move(cover),
          std::move(low), std::move(up)));
  return true;
}

bool fzn_global_cardinality_low_up_closed(
    FznInvariantGraph& invariantGraph,
    const std::shared_ptr<fznparser::IntVarArray>& inputs,
    std::vector<Int>&& cover, std::vector<Int>&& low, std::vector<Int>&& up,
    const fznparser::BoolArg& reified) {
  invariantGraph.addInvariantNode(
      std::make_unique<GlobalCardinalityLowUpClosedNode>(
          invariantGraph.retrieveVarNodes(inputs), std::move(cover),
          std::move(low), std::move(up),
          invariantGraph.retrieveVarNode(reified)));
  return true;
}

bool fzn_global_cardinality_low_up_closed(
    FznInvariantGraph& invariantGraph,
    const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "fzn_global_cardinality_low_up_closed" &&
      constraint.identifier() != "fzn_global_cardinality_low_up_closed_reif") {
    return false;
  }

  const bool isReified = constraintIdentifierIsReified(constraint);
  verifyNumArguments(constraint, isReified ? 4 : 3);
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::IntVarArray, true)
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 1, fznparser::IntVarArray, false)
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 2, fznparser::IntVarArray, false)
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 3, fznparser::IntVarArray, false)
  std::vector<Int> cover = std::get<std::shared_ptr<fznparser::IntVarArray>>(
                               constraint.arguments().at(1))
                               ->toParVector();
  std::vector<Int> low = std::get<std::shared_ptr<fznparser::IntVarArray>>(
                             constraint.arguments().at(2))
                             ->toParVector();
  std::vector<Int> up = std::get<std::shared_ptr<fznparser::IntVarArray>>(
                            constraint.arguments().at(3))
                            ->toParVector();
  if (!isReified) {
    return fzn_global_cardinality_low_up_closed(
        invariantGraph,
        std::get<std::shared_ptr<fznparser::IntVarArray>>(
            constraint.arguments().at(0)),
        std::move(cover), std::move(low), std::move(up));
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 3, fznparser::BoolArg, true)
  return fzn_global_cardinality_low_up_closed(
      invariantGraph,
      std::get<std::shared_ptr<fznparser::IntVarArray>>(
          constraint.arguments().at(0)),
      std::move(cover), std::move(low), std::move(up),
      std::get<fznparser::BoolArg>(constraint.arguments().at(3)));
}

}  // namespace atlantis::invariantgraph::fzn
