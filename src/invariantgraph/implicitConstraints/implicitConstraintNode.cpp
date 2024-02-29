#include "atlantis/invariantgraph/implicitConstraintNode.hpp"

#include "atlantis/invariantgraph/invariantGraph.hpp"

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
  std::vector<search::SearchVar> varIds;
  varIds.reserve(outputVarNodeIds().size());

  for (const auto& id : outputVarNodeIds()) {
    auto& node = invariantGraph.varNode(id);
    assert(node.varId() != propagation::NULL_ID);
    varIds.emplace_back(node.varId(), std::move(node.domain()));
    node.setDomainType(VarNode::DomainType::NONE);
  }

  _neighbourhood = createNeighbourhood(solver, std::move(varIds));
  assert(_neighbourhood);
}

}  // namespace atlantis::invariantgraph
