#include "invariantgraph/fzn/fzn_global_cardinality_closed.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"

namespace atlantis::invariantgraph::fzn {

void checkInputs(const std::vector<Int>& cover,
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

  std::vector<VarNodeId> inputVarNodeIds =
      invariantGraph.createVarNodes(inputs, false);
  for (VarNodeId varNodeId : inputVarNodeIds) {
    invariantGraph.varNode(varNodeId).removeAllValuesExcept(cover);
  }

  invariantGraph.addInvariantNode(
      std::move(std::make_unique<GlobalCardinalityNode>(
          std::move(invariantGraph.createVarNodes(inputs, false)),
          std::move(cover),
          std::move(invariantGraph.createVarNodes(counts, true)))));
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

  std::vector<VarNodeId> inputVarNodeIds =
      invariantGraph.createVarNodes(inputs, false);

  std::vector<VarNodeId> countVarNodeIds =
      invariantGraph.createVarNodes(counts, false);

  std::vector<VarNodeId> outputVarNodeIds;
  outputVarNodeIds.reserve(counts.size());
  for (size_t i = 0; i < counts.size(); ++i) {
    outputVarNodeIds.push_back(invariantGraph.createVarNode(
        SearchDomain(0, counts.size()), true, true));
  }

  std::vector<VarNodeId> violationVarNodeIds;
  violationVarNodeIds.reserve(counts.size() + 1);
  for (size_t i = 0; i <= counts.size(); ++i) {
    violationVarNodeIds.push_back(
        invariantGraph.createVarNode(SearchDomain(0, 1), false, true));
  }

  for (size_t i = 0; i < counts.size(); ++i) {
    int_eq(invariantGraph, countVarNodeIds.at(i), outputVarNodeIds.at(i),
           violationVarNodeIds.at(i));
  }

  invariantGraph.addInvariantNode(
      std::move(std::make_unique<GlobalCardinalityClosedNode>(
          std::move(inputVarNodeIds), std::move(cover),
          std::move(outputVarNodeIds), violationVarNodeIds.back())));

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
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntVarArray, true);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::IntVarArray, false);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 2, fznparser::IntVarArray, true);
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
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 3, fznparser::BoolArg, true);
  return fzn_global_cardinality_closed(
      invariantGraph,
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0)),
      std::move(cover),
      std::get<fznparser::IntVarArray>(constraint.arguments().at(2)),
      std::get<fznparser::BoolArg>(constraint.arguments().at(3)));
}

}  // namespace atlantis::invariantgraph::fzn