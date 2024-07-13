#include "atlantis/invariantgraph/fzn/fzn_global_cardinality.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/invariantNodes/globalCardinalityNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolAndNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolAllEqualNode.hpp"

namespace atlantis::invariantgraph::fzn {

static void checkInputs(const std::vector<Int>& cover,
                        const std::shared_ptr<fznparser::IntVarArray>& counts) {
  if (cover.size() != counts->size()) {
    throw FznArgumentException(
        "fzn_global_cardinality: cover and counts must have the same "
        "size");
  }
}

bool fzn_global_cardinality(
    FznInvariantGraph& graph,
    const std::shared_ptr<fznparser::IntVarArray>& inputs,
    std::vector<Int>&& cover,
    const std::shared_ptr<fznparser::IntVarArray>& counts) {
  checkInputs(cover, counts);

  graph.addInvariantNode(std::make_unique<GlobalCardinalityNode>(
      graph.retrieveVarNodes(inputs), std::move(cover),
      graph.retrieveVarNodes(counts)));
  return true;
}

bool fzn_global_cardinality(
    FznInvariantGraph& graph,
    const std::shared_ptr<fznparser::IntVarArray>& inputs,
    std::vector<Int>&& cover,
    const std::shared_ptr<fznparser::IntVarArray>& counts,
    const fznparser::BoolArg& reified) {
  checkInputs(cover, counts);
  if (reified.isFixed() && reified.toParameter()) {
    return fzn_global_cardinality(graph, inputs, std::move(cover), counts);
  }

  std::vector<VarNodeId> countVarNodeIds = graph.retrieveVarNodes(counts);
  std::vector<VarNodeId> outputVarNodeIds;
  std::vector<VarNodeId> binaryOutputVarNodeIds;
  outputVarNodeIds.reserve(counts->size());
  binaryOutputVarNodeIds.reserve(counts->size());
  for (size_t i = 0; i < counts->size(); ++i) {
    outputVarNodeIds.push_back(graph.retrieveIntVarNode(
        SearchDomain(0, static_cast<Int>(inputs->size())),
        VarNode::DomainType::NONE));
    binaryOutputVarNodeIds.push_back(graph.retrieveBoolVarNode());
    graph.addInvariantNode(std::make_unique<BoolAllEqualNode>(
        outputVarNodeIds.at(i), countVarNodeIds.at(i),
        binaryOutputVarNodeIds.at(i)));
  }

  graph.addInvariantNode(std::make_unique<GlobalCardinalityNode>(
      graph.retrieveVarNodes(inputs), std::move(cover),
      std::move(outputVarNodeIds)));

  graph.addInvariantNode(std::make_unique<ArrayBoolAndNode>(
      std::move(binaryOutputVarNodeIds), graph.retrieveVarNode(reified)));
  return true;
}

bool fzn_global_cardinality(FznInvariantGraph& graph,
                            const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "fzn_global_cardinality" &&
      constraint.identifier() != "fzn_global_cardinality_reif") {
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
    return fzn_global_cardinality(
        graph,
        getArgArray<fznparser::IntVarArray>(constraint.arguments().at(0)),
        std::move(cover),
        getArgArray<fznparser::IntVarArray>(constraint.arguments().at(2)));
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 3, fznparser::BoolArg, true)
  return fzn_global_cardinality(
      graph, getArgArray<fznparser::IntVarArray>(constraint.arguments().at(0)),
      std::move(cover),
      getArgArray<fznparser::IntVarArray>(constraint.arguments().at(2)),
      std::get<fznparser::BoolArg>(constraint.arguments().at(3)));
}

}  // namespace atlantis::invariantgraph::fzn
