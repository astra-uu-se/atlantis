#include "atlantis/invariantgraph/fzn/allDifferentImplicitNode.hpp"

#include <unordered_set>

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/implicitConstraintNodes/allDifferentImplicitNode.hpp"
#include "atlantis/invariantgraph/types.hpp"

namespace atlantis::invariantgraph::fzn {

bool makeAllDifferentImplicitNode(
    FznInvariantGraph& graph,
    const std::shared_ptr<fznparser::IntVarArray>& intVarArray) {
  if (intVarArray->size() <= 1) {
    return true;
  }

  verifyAllDifferent(intVarArray);

  if (intVarArray->isParArray()) {
    return true;
  }

  std::vector<VarNodeId> varNodeIds =
      pruneAllDifferentFree(graph, graph.retrieveVarNodes(intVarArray));

  graph.addImplicitConstraintNode(
      std::make_unique<AllDifferentImplicitNode>(std::move(varNodeIds)));
  return true;
}

bool makeAllDifferentImplicitNode(FznInvariantGraph& graph,
                                  const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "fzn_all_different_int") {
    return false;
  }

  verifyNumArguments(constraint, 1);
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 0, fznparser::IntVarArray, true)

  return makeAllDifferentImplicitNode(
      graph, getArgArray<fznparser::IntVarArray>(constraint.arguments().at(0)));
}

}  // namespace atlantis::invariantgraph::fzn
