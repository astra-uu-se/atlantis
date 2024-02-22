#include "invariantgraph/fzn/fzn_global_cardinality_closed.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"

namespace atlantis::invariantgraph::fzn {

static void checkInputs(const std::vector<Int>& cover,
                        const fznparser::IntVarArray& counts) {
  if (cover.size() != counts.size()) {
    throw FznArgumentException(
        "fzn_global_cardinality_closed: cover and counts must have the same "
        "size");
  }
}

bool fzn_global_cardinality_closed(FznInvariantGraph& invariantGraph,
                                   const fznparser::IntVarArray& inputs,
                                   std::vector<Int>&& cover,
                                   const fznparser::IntVarArray& counts) {
  checkInputs(cover, counts);

  std::vector<VarNodeId> inputVarNodeIds = invariantGraph.inputVarNodes(inputs);
  for (VarNodeId varNodeId : inputVarNodeIds) {
    invariantGraph.varNode(varNodeId).removeAllValuesExcept(cover);
  }

  invariantGraph.addInvariantNode(std::make_unique<GlobalCardinalityNode>(
      invariantGraph.inputVarNodes(inputs), std::move(cover),
      invariantGraph.defineVarNodes(counts)));
  return true;
}

bool fzn_global_cardinality_closed(FznInvariantGraph& invariantGraph,
                                   const fznparser::IntVarArray& inputs,
                                   std::vector<Int>&& cover,
                                   const fznparser::IntVarArray& counts,
                                   const fznparser::BoolArg& reified) {
  checkInputs(cover, counts);
  if (reified.isFixed() && reified.toParameter()) {
    return fzn_global_cardinality_closed(invariantGraph, inputs,
                                         std::move(cover), counts);
  }

  std::vector<VarNodeId> inputVarNodeIds = invariantGraph.inputVarNodes(inputs);

  std::vector<VarNodeId> countVarNodeIds = invariantGraph.inputVarNodes(counts);
  std::vector<VarNodeId> countVarNodeIds =
      invariantGraph.createVarNodes(counts, true);

  std::vector<VarNodeId> outputVarNodeIds;
  outputVarNodeIds.reserve(counts.size());

  std::vector<VarNodeId> violationVarNodeIds;
  violationVarNodeIds.reserve(inputs.size() + counts.size() + 1);

  for (VarNodeId countId : countVarNodeIds) {
    outputVarNodeIds.emplace_back(invariantGraph.createVarNode(
        SearchDomain(0, static_cast<Int>(counts.size())), true, true));

    violationVarNodeIds.emplace_back(
        invariantGraph.createVarNode(SearchDomain(0, 1), false, true));

    int_eq(invariantGraph, countId, outputVarNodeIds.back(),
           violationVarNodeIds.back());
  }

  for (VarNodeId inputId : inputVarNodeIds) {
    violationVarNodeIds.emplace_back(
        invariantGraph.createVarNode(SearchDomain(0, 1), false, true));

    set_in(invariantGraph, inputId, std::vector<Int>(cover),
           violationVarNodeIds.back());
  }

  invariantGraph.addInvariantNode(std::make_unique<GlobalCardinalityNode>(
      std::move(inputVarNodeIds), std::move(cover),
      std::move(outputVarNodeIds)));

  return array_bool_and(invariantGraph, std::move(violationVarNodeIds),
                        reified);
}

bool fzn_global_cardinality_closed(FznInvariantGraph& invariantGraph,
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
      std::get<fznparser::IntVarArray>(constraint.arguments().at(1))
          .toParVector();
  if (!isReified) {
    return fzn_global_cardinality_closed(
        invariantGraph,
        std::get<fznparser::IntVarArray>(constraint.arguments().at(0)),
        std::move(cover),
        std::get<fznparser::IntVarArray>(constraint.arguments().at(2)));
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 3, fznparser::BoolArg, true)
  return fzn_global_cardinality_closed(
      invariantGraph,
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0)),
      std::move(cover),
      std::get<fznparser::IntVarArray>(constraint.arguments().at(2)),
      std::get<fznparser::BoolArg>(constraint.arguments().at(3)));
}

}  // namespace atlantis::invariantgraph::fzn