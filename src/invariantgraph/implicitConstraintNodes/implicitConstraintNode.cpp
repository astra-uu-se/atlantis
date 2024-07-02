#include "atlantis/invariantgraph/implicitConstraintNode.hpp"

#include <cassert>
#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/search/neighbourhoods/neighbourhood.hpp"

namespace atlantis::invariantgraph {

ImplicitConstraintNode::ImplicitConstraintNode(
    std::vector<VarNodeId>&& outputVarNodeIds)
    : InvariantNode(std::move(outputVarNodeIds)) {}

void ImplicitConstraintNode::registerOutputVars(
    InvariantGraph& invariantGraph, propagation::SolverBase& solver) {
  for (const auto& varNodeId : outputVarNodeIds()) {
    auto& varNode = invariantGraph.varNode(varNodeId);
    if (varNode.varId() == propagation::NULL_ID) {
      const auto& [lb, ub] = varNode.bounds();
      varNode.setVarId(solver.makeIntVar(lb, lb, ub));
    }
  }
}

std::shared_ptr<search::neighbourhoods::Neighbourhood>
ImplicitConstraintNode::neighbourhood() noexcept {
  return _neighbourhood;
}

void ImplicitConstraintNode::registerNode(InvariantGraph& invariantGraph,
                                          propagation::SolverBase& solver) {
  if (_neighbourhood != nullptr) {
    return;
  }
  _neighbourhood = createNeighbourhood(invariantGraph, solver);
  assert(_neighbourhood);
}

}  // namespace atlantis::invariantgraph
