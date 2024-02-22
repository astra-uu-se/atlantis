#include "invariantgraph/fzn/allDifferentImplicitNode.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"

namespace atlantis::invariantgraph::fzn {

bool makeAllDifferentImplicitNode(FznInvariantGraph& invariantGraph,
                                  const fznparser::IntVarArray& intVarArray) {
  if (intVarArray.size() <= 1) {
    return true;
  }

  verifyAllDifferent(intVarArray);

  if (intVarArray.isParArray()) {
    return true;
  }

  std::vector<VarNodeId> varNodeIds = pruneAllDifferentFree(
      invariantGraph, invariantGraph.defineVarNodes(intVarArray));

  invariantGraph.addImplicitConstraintNode(
      std::make_unique<AllDifferentImplicitNode>(std::move(varNodeIds)));
  return true;
}

bool makeAllDifferentImplicitNode(FznInvariantGraph& invariantGraph,
                                  const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "fzn_all_different_int") {
    return false;
  }

  verifyNumArguments(constraint, 1);
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::IntVarArray, true)
  return makeAllDifferentImplicitNode(
      invariantGraph,
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0)));
}

}  // namespace atlantis::invariantgraph::fzn