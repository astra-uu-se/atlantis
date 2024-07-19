#include "atlantis/invariantgraph/fzn/fzn_global_cardinality_low_up.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/globalCardinalityLowUpNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool fzn_global_cardinality_low_up(
    FznInvariantGraph& graph,
    const std::shared_ptr<fznparser::IntVarArray>& inputs,
    std::vector<Int>&& cover, std::vector<Int>&& low, std::vector<Int>&& up) {
  graph.addInvariantNode(std::make_unique<GlobalCardinalityLowUpNode>(
      graph.retrieveVarNodes(inputs), std::move(cover), std::move(low),
      std::move(up)));
  return true;
}

bool fzn_global_cardinality_low_up(
    FznInvariantGraph& graph,
    const std::shared_ptr<fznparser::IntVarArray>& inputs,
    std::vector<Int>&& cover, std::vector<Int>&& low, std::vector<Int>&& up,
    const fznparser::BoolArg& reified) {
  graph.addInvariantNode(std::make_unique<GlobalCardinalityLowUpNode>(
      graph.retrieveVarNodes(inputs), std::move(cover), std::move(low),
      std::move(up), graph.retrieveVarNode(reified)));
  return true;
}

bool fzn_global_cardinality_low_up(FznInvariantGraph& graph,
                                   const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "fzn_global_cardinality_low_up" &&
      constraint.identifier() != "fzn_global_cardinality_low_up_reif") {
    return false;
  }

  const bool isReified = constraintIdentifierIsReified(constraint);
  verifyNumArguments(constraint, isReified ? 5 : 4);
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::IntVarArray, true)
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 1, fznparser::IntVarArray, false)
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 2, fznparser::IntVarArray, false)
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 3, fznparser::IntVarArray, false)
  std::vector<Int> cover =
      getArgArray<fznparser::IntVarArray>(constraint.arguments().at(1))
          ->toParVector();
  std::vector<Int> low =
      getArgArray<fznparser::IntVarArray>(constraint.arguments().at(2))
          ->toParVector();
  std::vector<Int> up =
      getArgArray<fznparser::IntVarArray>(constraint.arguments().at(3))
          ->toParVector();
  if (!isReified) {
    return fzn_global_cardinality_low_up(
        graph,
        getArgArray<fznparser::IntVarArray>(constraint.arguments().at(0)),
        std::move(cover), std::move(low), std::move(up));
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 4, fznparser::BoolArg, true)
  return fzn_global_cardinality_low_up(
      graph, getArgArray<fznparser::IntVarArray>(constraint.arguments().at(0)),
      std::move(cover), std::move(low), std::move(up),
      std::get<fznparser::BoolArg>(constraint.arguments().at(3)));
}

}  // namespace atlantis::invariantgraph::fzn
