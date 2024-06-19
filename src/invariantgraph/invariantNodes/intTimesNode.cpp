#include "atlantis/invariantgraph/invariantNodes/intTimesNode.hpp"

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/views/intScalarNode.hpp"
#include "atlantis/propagation/invariants/times.hpp"

namespace atlantis::invariantgraph {

IntTimesNode::IntTimesNode(VarNodeId a, VarNodeId b, VarNodeId output)
    : InvariantNode({output}, {a, b}) {}

void IntTimesNode::registerOutputVars(InvariantGraph& invariantGraph,
                                      propagation::SolverBase& solver) {
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()));
}

void IntTimesNode::updateState(InvariantGraph& graph) {
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());
  Int product = 1;

  for (const auto& input : staticInputVarNodeIds()) {
    if (graph.varNodeConst(input).isFixed()) {
      varsToRemove.emplace_back(input);
      product *= graph.varNodeConst(input).lowerBound();
    }
  }
  if (varsToRemove.size() == staticInputVarNodeIds().size()) {
    graph.varNode(outputVarNodeIds().front()).fixToValue(product);
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool IntTimesNode::canBeReplaced(const InvariantGraph& graph) const {
  return std::any_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const auto& input) { return graph.varNodeConst(input).isFixed(); });
}

bool IntTimesNode::replace(InvariantGraph& graph) {
  if (!canBeReplaced(graph)) {
    return false;
  }
  Int product = 1;
  VarNodeId unfixedVar{NULL_NODE_ID};
  for (const auto& input : staticInputVarNodeIds()) {
    if (graph.varNodeConst(input).isFixed()) {
      product *= graph.varNodeConst(input).lowerBound();
    } else {
      unfixedVar = input;
    }
  }
  if (unfixedVar != NULL_NODE_ID) {
    if (product == 1) {
      graph.replaceVarNode(outputVarNodeIds().front(), unfixedVar);
    } else {
      graph.addInvariantNode(std::make_unique<IntScalarNode>(
          unfixedVar, outputVarNodeIds().front(), product, 0));
    }
  }
  return true;
}

void IntTimesNode::registerNode(InvariantGraph& invariantGraph,
                                propagation::SolverBase& solver) {
  assert(invariantGraph.varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);
  solver.makeInvariant<propagation::Times>(
      solver, invariantGraph.varId(outputVarNodeIds().front()),
      invariantGraph.varId(a()), invariantGraph.varId(b()));
}

VarNodeId IntTimesNode::a() const { return staticInputVarNodeIds().front(); }
VarNodeId IntTimesNode::b() const { return staticInputVarNodeIds().back(); }

}  // namespace atlantis::invariantgraph
