#include "invariantgraph/fzn/fzn_all_different_int.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"

namespace atlantis::invariantgraph::fzn {

bool fzn_all_different_int(FznInvariantGraph& invariantGraph,
                           const fznparser::IntVarArray& inputs) {
  if (inputs.size() <= 1) {
    return true;
  }

  verifyAllDifferent(inputs);

  if (inputs.isParArray()) {
    return true;
  }

  std::vector<VarNodeId> varNodeIds = pruneAllDifferentFree(
      invariantGraph, invariantGraph.inputVarNodes(inputs));

  invariantGraph.addInvariantNode(
      std::make_unique<AllDifferentNode>(std::move(varNodeIds), true));
  return true;
}

bool fzn_all_different_int(FznInvariantGraph& invariantGraph,
                           const fznparser::IntVarArray& inputs,
                           const fznparser::BoolArg& reified) {
  if (reified.isFixed()) {
    if (reified.toParameter()) {
      return fzn_all_different_int(invariantGraph, inputs);
    }
    invariantGraph.addInvariantNode(std::make_unique<AllDifferentNode>(
        invariantGraph.inputVarNodes(inputs), reified.toParameter()));
    return true;
  }
  invariantGraph.addInvariantNode(std::make_unique<AllDifferentNode>(
      invariantGraph.inputVarNodes(inputs),
      invariantGraph.defineVarNode(reified)));
  return true;
}

bool fzn_all_different_int(FznInvariantGraph& invariantGraph,
                           const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "fzn_all_different_int" &&
      constraint.identifier() != "fzn_all_different_int_reif") {
    return false;
  }

  const bool isReified = constraintIdentifierIsReified(constraint);
  verifyNumArguments(constraint, isReified ? 2 : 1);
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::IntVarArray, true)

  if (!isReified) {
    return fzn_all_different_int(
        invariantGraph,
        std::get<fznparser::IntVarArray>(constraint.arguments().at(0)));
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::BoolArg, true)
  return fzn_all_different_int(
      invariantGraph,
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0)),
      std::get<fznparser::BoolArg>(constraint.arguments().at(1)));
}

}  // namespace atlantis::invariantgraph::fzn