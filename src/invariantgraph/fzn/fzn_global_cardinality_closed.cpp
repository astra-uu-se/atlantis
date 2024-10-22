#include "atlantis/invariantgraph/fzn/fzn_global_cardinality_closed.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/globalCardinalityClosedNode.hpp"

namespace atlantis::invariantgraph::fzn {

bool fzn_global_cardinality_closed(
    FznInvariantGraph& graph,
    const std::shared_ptr<fznparser::IntVarArray>& inputs,
    std::vector<Int>&& cover,
    const std::shared_ptr<fznparser::IntVarArray>& counts) {
  graph.addInvariantNode(std::make_shared<GlobalCardinalityClosedNode>(
      graph, graph.retrieveVarNodes(inputs), std::move(cover),
      graph.retrieveVarNodes(counts)));
  return true;
}

bool fzn_global_cardinality_closed(
    FznInvariantGraph& graph,
    const std::shared_ptr<fznparser::IntVarArray>& inputs,
    std::vector<Int>&& cover,
    const std::shared_ptr<fznparser::IntVarArray>& counts,
    const fznparser::BoolArg& reified) {
  graph.addInvariantNode(std::make_shared<GlobalCardinalityClosedNode>(
      graph, graph.retrieveVarNodes(inputs), std::move(cover),
      graph.retrieveVarNodes(counts), graph.retrieveVarNode(reified)));
  return true;
}

bool fzn_global_cardinality_closed(FznInvariantGraph& graph,
                                   const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "fzn_global_cardinality_closed" &&
      constraint.identifier() != "fzn_global_cardinality_closed_reif") {
    return false;
  }

  const bool isReified = constraintIdentifierIsReified(constraint);
  verifyNumArguments(constraint, isReified ? 4 : 3);
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::IntVarArray, true)
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 1, fznparser::IntVarArray, false)
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 2, fznparser::IntVarArray, true)
  std::vector<Int> cover =
      getArgArray<fznparser::IntVarArray>(constraint.arguments().at(1))
          ->toParVector();
  if (!isReified) {
    return fzn_global_cardinality_closed(
        graph,
        getArgArray<fznparser::IntVarArray>(constraint.arguments().at(0)),
        std::move(cover),
        getArgArray<fznparser::IntVarArray>(constraint.arguments().at(2)));
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 3, fznparser::BoolArg, true)
  return fzn_global_cardinality_closed(
      graph, getArgArray<fznparser::IntVarArray>(constraint.arguments().at(0)),
      std::move(cover),
      getArgArray<fznparser::IntVarArray>(constraint.arguments().at(2)),
      std::get<fznparser::BoolArg>(constraint.arguments().at(3)));
}

}  // namespace atlantis::invariantgraph::fzn
