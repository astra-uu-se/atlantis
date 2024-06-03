#include "atlantis/invariantgraph/invariantNodes/arrayIntMaximumNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/maxSparse.hpp"

namespace atlantis::invariantgraph {

ArrayIntMaximumNode::ArrayIntMaximumNode(std::vector<VarNodeId>&& vars,
                                         VarNodeId output)
    : InvariantNode({output}, std::move(vars)) {}

void ArrayIntMaximumNode::registerOutputVars(InvariantGraph& invariantGraph,
                                             propagation::SolverBase& solver) {
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()));
}

void ArrayIntMaximumNode::propagate(InvariantGraph& invariantGraph) {
  Int lb = invariantGraph.lowerBound(outputVarNodeIds().front());
  const Int ub = invariantGraph.upperBound(outputVarNodeIds().front());

  for (const VarNodeId& nodeId : staticInputVarNodeIds()) {
    if (invariantGraph.lowerBound(nodeId) > ub) {
      setState(InvariantNodeState::INFEASIBLE);
      return;
    }
    lb = std::max(lb, invariantGraph.lowerBound(nodeId));
    invariantGraph.removeValuesAbove(nodeId, ub);
  }

  if (lb > ub) {
    setState(InvariantNodeState::INFEASIBLE);
    return;
  }

  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());

  for (const VarNodeId& nodeId : staticInputVarNodeIds()) {
    if (invariantGraph.varNode(nodeId).lowerBound() <= lb) {
      varsToRemove.emplace_back(nodeId);
    }
  }

  for (const VarNodeId& nodeId : varsToRemove) {
    removeStaticInputVarNode(invariantGraph.varNode(nodeId));
  }

  if (staticInputVarNodeIds().empty()) {
    invariantGraph.fixToValue(outputVarNodeIds().front(), lb);
    setState(InvariantNodeState::SUBSUMED);
    return;
  }

  invariantGraph.removeValuesBelow(outputVarNodeIds().front(), lb);

  if (staticInputVarNodeIds().size() == 1) {
    setState(InvariantNodeState::REPLACABLE);
  }
}

bool ArrayIntMaximumNode::replace(InvariantGraph& invariantGraph) {
  if (state() != InvariantNodeState::REPLACABLE) {
    return false;
  }
  assert(staticInputVarNodeIds().size() == 1);
  invariantGraph.replaceVarNode(staticInputVarNodeIds().front(),
                                outputVarNodeIds().front());
  deactivate(invariantGraph);
  return true;
}

void ArrayIntMaximumNode::registerNode(InvariantGraph& invariantGraph,
                                       propagation::SolverBase& solver) {
  std::vector<propagation::VarId> solverVars;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars),
                 [&](const auto& node) { return invariantGraph.varId(node); });

  assert(invariantGraph.varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);
  solver.makeInvariant<propagation::MaxSparse>(
      solver, invariantGraph.varId(outputVarNodeIds().front()),
      std::move(solverVars));
}

}  // namespace atlantis::invariantgraph
