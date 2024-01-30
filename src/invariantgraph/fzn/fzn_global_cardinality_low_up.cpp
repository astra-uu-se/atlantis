#include "invariantgraph/fzn/fzn_global_cardinality_low_up.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"

namespace atlantis::invariantgraph::fzn {

static void checkParams(const std::vector<Int>& cover, const std::vector<Int>& low,
                 const std::vector<Int>& up) {
  if (cover.size() != low.size() || cover.size() != up.size()) {
    throw FznArgumentException(
        "fzn_global_cardinality_low_up: cover, low and up must have the "
        "same size");
  }
  for (size_t i = 0; i < cover.size(); ++i) {
    if (low[i] > up[i]) {
      throw FznArgumentException(
          "fzn_global_cardinality_low_up: low[" + std::to_string(i) +
          "] must be less than or equal to up[" + std::to_string(i) + "].");
    }
  }
}

bool fzn_global_cardinality_low_up(FznInvariantGraph& invariantGraph,
                                   const fznparser::IntVarArray& inputs,
                                   std::vector<Int>&& cover,
                                   std::vector<Int>&& low,
                                   std::vector<Int>&& up) {
  checkParams(cover, low, up);
  for (size_t i = 0; i < cover.size(); ++i) {
    if (low[i] > up[i]) {
      throw FznArgumentException(
          "fzn_global_cardinality_low_up: low[" + std::to_string(i) +
          "] must be less than or equal to up[" + std::to_string(i) + "].");
    }
  }

  invariantGraph.addInvariantNode(
      std::make_unique<GlobalCardinalityLowUpNode>(
          invariantGraph.createVarNodes(inputs, false),
          std::move(cover), std::move(low), std::move(up), true));
  return true;
}

bool fzn_global_cardinality_low_up(FznInvariantGraph& invariantGraph,
                                   const fznparser::IntVarArray& inputs,
                                   std::vector<Int>&& cover,
                                   std::vector<Int>&& low,
                                   std::vector<Int>&& up,
                                   const fznparser::BoolArg& reified) {
  checkParams(cover, low, up);
  if (reified.isFixed()) {
    if (reified.toParameter()) {
      return fzn_global_cardinality_low_up(invariantGraph, inputs,
                                           std::move(cover), std::move(low),
                                           std::move(up));
    }
    invariantGraph.addInvariantNode(
        std::make_unique<GlobalCardinalityLowUpNode>(
            invariantGraph.createVarNodes(inputs, false),
            std::move(cover), std::move(low), std::move(up),
            reified.toParameter()));
    return true;
  }
  invariantGraph.addInvariantNode(
      std::make_unique<GlobalCardinalityLowUpNode>(
          invariantGraph.createVarNodes(inputs, false),
          std::move(cover), std::move(low), std::move(up), true));
  return true;
}

bool fzn_global_cardinality_low_up(FznInvariantGraph& invariantGraph,
                                   const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "fzn_global_cardinality_low_up" &&
      constraint.identifier() != "fzn_global_cardinality_low_up_reif") {
    return false;
  }

  const bool isReified = constraintIdentifierIsReified(constraint);
  verifyNumArguments(constraint, isReified ? 4 : 3);
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::IntVarArray, true)
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 1, fznparser::IntVarArray, false)
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 2, fznparser::IntVarArray, false)
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 3, fznparser::IntVarArray, false)
  std::vector<Int> cover =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(1))
          .toParVector();
  std::vector<Int> low =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(2))
          .toParVector();
  std::vector<Int> up =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(3))
          .toParVector();
  if (!isReified) {
    return fzn_global_cardinality_low_up(
        invariantGraph,
        std::get<fznparser::IntVarArray>(constraint.arguments().at(0)),
        std::move(cover), std::move(low), std::move(up));
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 3, fznparser::BoolArg, true)
  return fzn_global_cardinality_low_up(
      invariantGraph,
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0)),
      std::move(cover), std::move(low), std::move(up),
      std::get<fznparser::BoolArg>(constraint.arguments().at(3)));
}

}  // namespace atlantis::invariantgraph::fzn