#include "atlantis/invariantgraph/invariantNodes/arrayIntMaximumNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/maxSparse.hpp"

namespace atlantis::invariantgraph {

ArrayIntMaximumNode::ArrayIntMaximumNode(VarNodeId a, VarNodeId b,
                                         VarNodeId output)
    : ArrayIntMaximumNode(std::vector<VarNodeId>{a, b}, output) {}

ArrayIntMaximumNode::ArrayIntMaximumNode(std::vector<VarNodeId>&& vars,
                                         VarNodeId output)
    : InvariantNode({output}, std::move(vars)) {}

void ArrayIntMaximumNode::registerOutputVars(InvariantGraph& invariantGraph,
                                             propagation::SolverBase& solver) {
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()));
}

void ArrayIntMaximumNode::updateState(InvariantGraph& graph) {
  Int min = graph.varNodeConst(outputVarNodeIds().front()).lowerBound();
  Int max = std::numeric_limits<Int>::min();
  for (const auto& input : staticInputVarNodeIds()) {
    min = std::min(min, graph.varNodeConst(input).upperBound());
    max = std::max(max, graph.varNodeConst(input).upperBound());
  }
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());

  for (const auto& input : staticInputVarNodeIds()) {
    if (graph.varNodeConst(input).upperBound() < min) {
      varsToRemove.emplace_back(input);
    }
  }
  for (const auto& input : varsToRemove) {
    removeStaticInputVarNode(graph.varNode(input));
  }
  auto& outputVar = graph.varNode(outputVarNodeIds().front());
  outputVar.removeValuesBelow(min);
  outputVar.removeValuesAbove(max);
  if (staticInputVarNodeIds().size() == 0 || outputVar.isFixed()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool ArrayIntMaximumNode::canBeReplaced(const InvariantGraph&) const {
  return staticInputVarNodeIds().size() <= 1;
}

bool ArrayIntMaximumNode::replace(InvariantGraph& invariantGraph) {
  if (!canBeReplaced(invariantGraph)) {
    return false;
  }
  if (staticInputVarNodeIds().size() == 1) {
    invariantGraph.replaceVarNode(outputVarNodeIds().front(),
                                  staticInputVarNodeIds().front());
  }
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
