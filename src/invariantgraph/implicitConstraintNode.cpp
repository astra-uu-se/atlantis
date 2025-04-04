#include "atlantis/invariantgraph/implicitConstraintNode.hpp"

#include <cassert>

#include "atlantis/invariantgraph/iInvariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/search/neighborhoods/neighborhood.hpp"

namespace atlantis::invariantgraph {

ImplicitConstraintNode::ImplicitConstraintNode(
    IInvariantGraph& graph, std::vector<VarNodeId>&& outputVarNodeIds)
    : InvariantNode(graph, std::move(outputVarNodeIds)) {}

void ImplicitConstraintNode::registerOutputVars() {
  for (const auto& varNodeId : outputVarNodeIds()) {
    auto& varNode = invariantGraph().varNode(varNodeId);
    if (varNode.varId() == propagation::NULL_ID) {
      const auto& [lb, ub] = varNode.bounds();
      varNode.setVarId(invariantGraph().solver().makeIntVar(lb, lb, ub));
    }
  }
}

void ImplicitConstraintNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
}

std::shared_ptr<search::neighborhoods::Neighborhood>
ImplicitConstraintNode::neighborhood() {
  return _neighborhood;
}

void ImplicitConstraintNode::registerNode() {
  if (_neighborhood != nullptr) {
    return;
  }
  _neighborhood = createNeighborhood();
  assert(_neighborhood);
}

}  // namespace atlantis::invariantgraph
