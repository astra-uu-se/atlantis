

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

  invariantGraph.addInvariantNode(
      std::move(std::make_unique<GlobalCardinalityClosedNode>(
          std::move(invariantGraph.createVarNodes(inputs, false)),
          std::move(cover),
          std::move(invariantGraph.createVarNodes(counts, true)), true)));
  return true;
}

bool fzn_global_cardinality_closed(FznInvariantGraph& invariantGraph,
                                   const fznparser::IntVarArray& inputs,
                                   std::vector<Int>&& cover,
                                   const fznparser::IntVarArray& counts,
                                   const fznparser::BoolArg& reified) {
  checkInputs(cover, counts);
  if (reified.isFixed()) {
    if (reified.toParameter()) {
      return fzn_global_cardinality_closed(invariantGraph, inputs,
                                           std::move(cover), counts);
    }
    invariantGraph.addInvariantNode(
        std::move(std::make_unique<GlobalCardinalityClosedNode>(
            std::move(invariantGraph.createVarNodes(inputs, false)),
            std::move(cover),
            std::move(invariantGraph.createVarNodes(counts, true)),
            reified.toParameter())));
    return true;
  }
  invariantGraph.addInvariantNode(
      std::move(std::make_unique<GlobalCardinalityClosedNode>(
          std::move(invariantGraph.createVarNodes(inputs, false)),
          std::move(cover),
          std::move(invariantGraph.createVarNodes(counts, true)),
          invariantGraph.createVarNodeFromFzn(reified, true))));
  return true;
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