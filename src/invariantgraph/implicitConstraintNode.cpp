#include "invariantgraph/implicitConstraintNode.hpp"

#include "invariantgraph/invariantGraph.hpp"

namespace invariantgraph {

void ImplicitConstraintNode::registerOutputVariables(
    InvariantGraph& invariantGraph, Engine& engine) {
  for (const auto& varNodeId : outputVarNodeIds()) {
    auto& varNode = invariantGraph.varNode(varNodeId);
    if (varNode.varId() == NULL_ID) {
      const auto& [lb, ub] = varNode.bounds();
      varNode.setVarId(engine.makeIntVar(lb, lb, ub));
    }
  }
}

std::unique_ptr<search::neighbourhoods::Neighbourhood>
ImplicitConstraintNode::takeNeighbourhood() noexcept {
  auto ptr =
      std::unique_ptr<search::neighbourhoods::Neighbourhood>(_neighbourhood);
  _neighbourhood = nullptr;
  return ptr;
}

void ImplicitConstraintNode::registerNode(InvariantGraph& invariantGraph,
                                          Engine& engine) {
  std::vector<search::SearchVariable> varIds;
  varIds.reserve(outputVarNodeIds().size());

  for (const auto& id : outputVarNodeIds()) {
    auto& node = invariantGraph.varNode(id);
    assert(node.varId() != NULL_ID);
    varIds.emplace_back(node.varId(), node.domain());
  }

  _neighbourhood = createNeighbourhood(engine, varIds);
  assert(_neighbourhood);
}

}  // namespace invariantgraph