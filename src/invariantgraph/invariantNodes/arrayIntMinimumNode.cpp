#include "atlantis/invariantgraph/invariantNodes/arrayIntMinimumNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/minSparse.hpp"

namespace atlantis::invariantgraph {

ArrayIntMinimumNode::ArrayIntMinimumNode(VarNodeId a, VarNodeId b,
                                         VarNodeId output)
    : ArrayIntMinimumNode(std::vector<VarNodeId>{a, b}, output) {}

ArrayIntMinimumNode::ArrayIntMinimumNode(std::vector<VarNodeId>&& vars,
                                         VarNodeId output)
    : InvariantNode({output}, std::move(vars)) {}

void ArrayIntMinimumNode::registerOutputVars(InvariantGraph& invariantGraph,
                                             propagation::SolverBase& solver) {
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()));
}

void ArrayIntMinimumNode::updateState(InvariantGraph& graph) {
  Int max = graph.varNodeConst(outputVarNodeIds().front()).upperBound();
  for (const auto& input : staticInputVarNodeIds()) {
    max = std::max(max, graph.varNodeConst(input).lowerBound());
  }
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());

  for (const auto& input : staticInputVarNodeIds()) {
    if (graph.varNodeConst(input).lowerBound() > max) {
      varsToRemove.emplace_back(input);
    }
  }
  for (const auto& input : varsToRemove) {
    removeStaticInputVarNode(graph.varNode(input));
  }
  if (staticInputVarNodeIds().size() == 0) {
    setState(InvariantNodeState::SUBSUMED);
  }
  graph.varNode(outputVarNodeIds().front()).removeValuesAbove(max);
}

bool ArrayIntMinimumNode::canBeReplaced(const InvariantGraph&) const {
  return staticInputVarNodeIds().size() <= 1;
}

bool ArrayIntMinimumNode::replace(InvariantGraph& invariantGraph) {
  if (!canBeReplaced(invariantGraph)) {
    return false;
  }
  if (staticInputVarNodeIds().size() == 1) {
    invariantGraph.replaceVarNode(outputVarNodeIds().front(),
                                  staticInputVarNodeIds().front());
  }
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
