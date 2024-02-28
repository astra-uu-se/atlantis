

#include "atlantis/invariantgraph/fzn/fzn_global_cardinality.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"

namespace atlantis::invariantgraph::fzn {

static void checkInputs(const std::vector<Int>& cover,
                        const fznparser::IntVarArray& counts) {
  if (cover.size() != counts.size()) {
    throw FznArgumentException(
        "fzn_global_cardinality: cover and counts must have the same "
        "size");
  }
}

bool fzn_global_cardinality(FznInvariantGraph& invariantGraph,
                            const fznparser::IntVarArray& inputs,
                            std::vector<Int>&& cover,
                            const fznparser::IntVarArray& counts) {
  checkInputs(cover, counts);

  invariantGraph.addInvariantNode(std::make_unique<GlobalCardinalityNode>(
      invariantGraph.retrieveVarNodes(inputs), std::move(cover),
      invariantGraph.retrieveVarNodes(counts)));
  return true;
}

bool fzn_global_cardinality(FznInvariantGraph& invariantGraph,
                            const fznparser::IntVarArray& inputs,
                            std::vector<Int>&& cover,
                            const fznparser::IntVarArray& counts,
                            const fznparser::BoolArg& reified) {
  checkInputs(cover, counts);
  if (reified.isFixed() && reified.toParameter()) {
    return fzn_global_cardinality(invariantGraph, inputs, std::move(cover),
                                  counts);
  }

  std::vector<VarNodeId> countVarNodeIds =
      invariantGraph.retrieveVarNodes(counts);
  std::vector<VarNodeId> outputVarNodeIds;
  std::vector<VarNodeId> binaryOutputVarNodeIds;
  outputVarNodeIds.reserve(counts.size());
  binaryOutputVarNodeIds.reserve(counts.size());
  for (size_t i = 0; i < counts.size(); ++i) {
    outputVarNodeIds.push_back(invariantGraph.retrieveIntVarNode(
        SearchDomain(0, static_cast<Int>(inputs.size())),
        VarNode::DomainType::NONE));
    binaryOutputVarNodeIds.push_back(invariantGraph.retrieveBoolVarNode());
    bool_eq(invariantGraph, outputVarNodeIds.at(i), countVarNodeIds.at(i),
            binaryOutputVarNodeIds.at(i));
  }

  invariantGraph.addInvariantNode(std::make_unique<GlobalCardinalityNode>(
      invariantGraph.retrieveVarNodes(inputs), std::move(cover),
      std::move(outputVarNodeIds)));

  array_bool_and(invariantGraph, std::move(binaryOutputVarNodeIds), reified);
  return true;
}

bool fzn_global_cardinality(FznInvariantGraph& invariantGraph,
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
      std::get<fznparser::IntVarArray>(constraint.arguments().at(1))
          .toParVector();
  if (!isReified) {
    return fzn_global_cardinality(
        invariantGraph,
        std::get<fznparser::IntVarArray>(constraint.arguments().at(0)),
        std::move(cover),
        std::get<fznparser::IntVarArray>(constraint.arguments().at(2)));
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 3, fznparser::BoolArg, true)
  return fzn_global_cardinality(
      invariantGraph,
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0)),
      std::move(cover),
      std::get<fznparser::IntVarArray>(constraint.arguments().at(2)),
      std::get<fznparser::BoolArg>(constraint.arguments().at(3)));
}

}  // namespace atlantis::invariantgraph::fzn
