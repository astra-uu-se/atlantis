#include "atlantis/invariantgraph/implicitConstraintNode.hpp"

#include <cassert>

#include "atlantis/invariantgraph/iInvariantGraph.hpp"
#include "atlantis/search/neighbourhoods/neighbourhood.hpp"

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

std::shared_ptr<search::neighbourhoods::Neighbourhood>
ImplicitConstraintNode::neighbourhood() {
  return _neighbourhood;
}

void ImplicitConstraintNode::registerNode() {
  if (_neighbourhood != nullptr) {
    return;
  }
  _neighbourhood = createNeighbourhood();
  assert(_neighbourhood);
}

}  // namespace atlantis::invariantgraph
