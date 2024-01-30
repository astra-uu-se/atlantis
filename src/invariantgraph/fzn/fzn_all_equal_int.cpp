

#include "invariantgraph/fzn/fzn_all_equal_int.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"

namespace atlantis::invariantgraph::fzn {

bool fzn_all_equal_int(FznInvariantGraph& invariantGraph,
                       const fznparser::IntVarArray& inputs) {
  if (inputs.size() <= 1) {
    return true;
  }

  if (violatesAllEqual(inputs)) {
    throw FznArgumentException(
        "fzn_all_equal_int: All fixed variables and parameters must take the "
        "same value");
  }

  if (inputs.isParArray()) {
    return true;
  }

  const VarNodeId allDiffViolVarNodeId = invariantGraph.createVarNode(
      SearchDomain(1, static_cast<Int>(inputs.size()) - 1), true, true);

  invariantGraph.addInvariantNode(std::make_unique<AllDifferentNode>(
      invariantGraph.createVarNodes(inputs, false),
      allDiffViolVarNodeId));

  invariantGraph.varNode(allDiffViolVarNodeId).shouldEnforceDomain(true);

  return true;
}

bool fzn_all_equal_int(FznInvariantGraph& invariantGraph,
                       const fznparser::IntVarArray& inputs,
                       const fznparser::BoolArg& reified) {
  if (reified.isFixed()) {
    if (reified.toParameter()) {
      return fzn_all_equal_int(invariantGraph, inputs);
    }
    // At least two variables must take different values
    const VarNodeId allDiffViolVarNodeId = invariantGraph.createVarNode(
        SearchDomain(0, static_cast<Int>(inputs.size()) - 2), false, true);

    invariantGraph.addInvariantNode(std::make_unique<AllDifferentNode>(
        invariantGraph.createVarNodes(inputs, false),
        allDiffViolVarNodeId));

    invariantGraph.varNode(allDiffViolVarNodeId).shouldEnforceDomain(true);

    return true;
  }

  const VarNodeId allDiffViolVarNodeId = invariantGraph.createVarNode(
      SearchDomain(0, static_cast<Int>(inputs.size()) - 1), false, true);

  invariantGraph.addInvariantNode(std::make_unique<AllDifferentNode>(
      invariantGraph.createVarNodes(inputs, false),
      allDiffViolVarNodeId));

  // If all variables take the same value (violation equals inputs.size() -
  // 1), then all_equal holds and the reified variable is true:
  int_eq(invariantGraph, allDiffViolVarNodeId,
         static_cast<Int>(inputs.size()) - 1, reified);
  return true;
}

bool fzn_all_equal_int(FznInvariantGraph& invariantGraph,
                       const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "fzn_all_equal_int" &&
      constraint.identifier() != "fzn_all_equal_int_reif") {
    return false;
  }

  const bool isReified = constraintIdentifierIsReified(constraint);
  verifyNumArguments(constraint, isReified ? 2 : 1);
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::IntVarArray, true)

  if (!isReified) {
    return fzn_all_equal_int(invariantGraph, std::get<fznparser::IntVarArray>(
                                                 constraint.arguments().at(0)));
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::BoolArg, true)
  return fzn_all_equal_int(
      invariantGraph,
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0)),
      std::get<fznparser::BoolArg>(constraint.arguments().at(1)));
}

}  // namespace atlantis::invariantgraph::fzn