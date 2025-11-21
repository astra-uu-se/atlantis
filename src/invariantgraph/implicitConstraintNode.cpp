#include "atlantis/invariantgraph/implicitConstraintNode.hpp"

#include <cassert>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/search/neighborhoods/neighborhood.hpp"

namespace atlantis::invariantgraph {

ImplicitConstraintNode::ImplicitConstraintNode(
    InvariantGraph& graph, std::vector<VarNodeId>&& outputVarNodeIds)
    : InvariantNode(graph, std::move(outputVarNodeIds)) {}

void ImplicitConstraintNode::registerOutputVars(propagation::SolverBase& solver,
                                                SolverMapping& mapping) const {
  for (const auto& varNodeId : outputVarNodeIds()) {
    const auto& varNode = invariantGraphConst().varNodeConst(varNodeId);
    if (mapping.solverId(varNodeId) == propagation::NULL_ID) {
      const auto& [lb, ub] = varNode.bounds();
      mapping.setSolverId(varNodeId, solver.makeIntVar(lb, lb, ub));
    }
  }
}

void ImplicitConstraintNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
}

}  // namespace atlantis::invariantgraph
