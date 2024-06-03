#include "atlantis/invariantgraph/invariantNodes/arrayIntMinimumNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/minSparse.hpp"

namespace atlantis::invariantgraph {

ArrayIntMinimumNode::ArrayIntMinimumNode(std::vector<VarNodeId>&& vars,
                                         VarNodeId output)
    : InvariantNode({output}, std::move(vars)) {}

void ArrayIntMinimumNode::registerOutputVars(InvariantGraph& invariantGraph,
                                             propagation::SolverBase& solver) {
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()));
}

void ArrayIntMinimumNode::propagate(InvariantGraph& invariantGraph) {
  const Int lb = invariantGraph.lowerBound(outputVarNodeIds().front());
  Int ub = invariantGraph.upperBound(outputVarNodeIds().front());

  for (const VarNodeId& nodeId : staticInputVarNodeIds()) {
    if (invariantGraph.upperBound(nodeId) < lb) {
      setState(InvariantNodeState::INFEASIBLE);
      return;
    }
    ub = std::min(ub, invariantGraph.upperBound(nodeId));
    invariantGraph.removeValuesBelow(nodeId, lb);
  }

  if (lb > ub) {
    setState(InvariantNodeState::INFEASIBLE);
    return;
  }

  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());

  for (const VarNodeId& nodeId : staticInputVarNodeIds()) {
    if (invariantGraph.upperBound(nodeId) >= ub) {
      varsToRemove.emplace_back(nodeId);
    }
  }

  for (const VarNodeId& nodeId : varsToRemove) {
    removeStaticInputVarNode(invariantGraph.varNode(nodeId));
  }

  if (staticInputVarNodeIds().empty()) {
    invariantGraph.fixToValue(outputVarNodeIds().front(), ub);
    setState(InvariantNodeState::SUBSUMED);
    return;
  }

  invariantGraph.removeValuesAbove(outputVarNodeIds().front(), ub);
  if (staticInputVarNodeIds().size() == 1) {
    setState(InvariantNodeState::REPLACABLE);
  }
}

bool ArrayIntMinimumNode::replace(InvariantGraph& invariantGraph) {
  if (state() != InvariantNodeState::REPLACABLE) {
    return false;
  }
  assert(staticInputVarNodeIds().size() == 1);
  invariantGraph.replaceVarNode(outputVarNodeIds().front(),
                                staticInputVarNodeIds().front());
  deactivate(invariantGraph);
  return true;
}

void ArrayIntMinimumNode::registerNode(InvariantGraph& invariantGraph,
                                       propagation::SolverBase& solver) {
  std::vector<propagation::VarId> solverVars;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars),
                 [&](const auto& node) { return invariantGraph.varId(node); });

  assert(invariantGraph.varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);
  solver.makeInvariant<propagation::MinSparse>(
      solver, invariantGraph.varId(outputVarNodeIds().front()),
      std::move(solverVars));
}

}  // namespace atlantis::invariantgraph
