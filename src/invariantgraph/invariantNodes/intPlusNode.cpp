#include "atlantis/invariantgraph/invariantNodes/intPlusNode.hpp"

#include <cmath>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/views/intScalarNode.hpp"
#include "atlantis/propagation/invariants/plus.hpp"

namespace atlantis::invariantgraph {

IntPlusNode::IntPlusNode(VarNodeId a, VarNodeId b, VarNodeId output)
    : InvariantNode({output}, {a, b}) {}

void IntPlusNode::updateState(InvariantGraph& graph) {
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());
  Int sum = 0;

  for (const auto& input : staticInputVarNodeIds()) {
    if (graph.varNodeConst(input).isFixed()) {
      varsToRemove.emplace_back(input);
      sum += graph.varNodeConst(input).lowerBound();
    }
  }
  if (varsToRemove.size() == staticInputVarNodeIds().size()) {
    graph.varNode(outputVarNodeIds().front()).fixToValue(sum);
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool IntPlusNode::canBeReplaced(const InvariantGraph& graph) const {
  return std::any_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const auto& input) { return graph.varNodeConst(input).isFixed(); });
}

bool IntPlusNode::replace(InvariantGraph& graph) {
  if (!canBeReplaced(graph)) {
    return false;
  }
  Int sum = 0;
  VarNodeId unfixedVar{NULL_NODE_ID};
  for (const auto& input : staticInputVarNodeIds()) {
    if (graph.varNodeConst(input).isFixed()) {
      sum += graph.varNodeConst(input).lowerBound();
    } else {
      unfixedVar = input;
    }
  }
  if (unfixedVar != NULL_NODE_ID) {
    if (sum == 0) {
      graph.replaceVarNode(outputVarNodeIds().front(), unfixedVar);
    } else {
      graph.addInvariantNode(std::make_unique<IntScalarNode>(
          unfixedVar, outputVarNodeIds().front(), 1, sum));
    }
  }
  return true;
}

void IntPlusNode::registerOutputVars(InvariantGraph& invariantGraph,
                                     propagation::SolverBase& solver) {
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()));
}

void IntPlusNode::registerNode(InvariantGraph& invariantGraph,
                               propagation::SolverBase& solver) {
  assert(invariantGraph.varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);
  solver.makeInvariant<propagation::Plus>(
      solver, invariantGraph.varId(outputVarNodeIds().front()),
      invariantGraph.varId(a()), invariantGraph.varId(b()));
}

}  // namespace atlantis::invariantgraph
