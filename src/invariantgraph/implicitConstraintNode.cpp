#include "atlantis/invariantgraph/implicitConstraintNode.hpp"

#include <cassert>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/search/neighborhoods/neighborhood.hpp"

namespace atlantis::invariantgraph {

ImplicitConstraintNode::ImplicitConstraintNode(
    InvariantGraph& graph,
    std::vector<std::shared_ptr<VarNode>>&& outputVarNodes)
    : InvariantNode(graph, std::move(outputVarNodes)) {}

void ImplicitConstraintNode::registerOutputVars(propagation::SolverBase& solver,
                                                SolverMapping& mapping) const {
  for (const auto& varNodeId : outputVarNodes()) {
    const auto& varNode = varNodeId;
    if (mapping.solverId(*varNodeId) == propagation::NULL_ID) {
      const auto& [lb, ub] = varNode->bounds();
      mapping.setSolverId(*varNodeId, solver.makeIntVar(lb, lb, ub));
    }
  }
}

void ImplicitConstraintNode::init() { InvariantNode::init(); }

bool ImplicitConstraintNode::constrainsOutput(const VarNode&) const {
  return true;
}

}  // namespace atlantis::invariantgraph
