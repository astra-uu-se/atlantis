#include "atlantis/invariantgraph/views/intAbsNode.hpp"

#include <map>
#include <utility>

#include "atlantis/propagation/views/intAbsView.hpp"

namespace atlantis::invariantgraph {

IntAbsNode::IntAbsNode(VarNodeId staticInput, VarNodeId output)
    : InvariantNode({output}, {staticInput}) {}

void IntAbsNode::updateState(InvariantGraph& graph) {
  if (graph.varNodeConst(staticInputVarNodeIds().front()).isFixed()) {
    graph.varNode(outputVarNodeIds().front())
        .fixToValue(std::abs(
            graph.varNodeConst(staticInputVarNodeIds().front()).lowerBound()));
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool IntAbsNode::canBeReplaced(const InvariantGraph& graph) const {
  return state() == InvariantNodeState::ACTIVE &&
         graph.varNodeConst(staticInputVarNodeIds().front()).lowerBound() >= 0;
}

bool IntAbsNode::replace(InvariantGraph& graph) {
  if (!canBeReplaced(graph)) {
    return false;
  }
  graph.replaceVarNode(outputVarNodeIds().front(),
                       staticInputVarNodeIds().front());
  return true;
}

void IntAbsNode::registerOutputVars(InvariantGraph& invariantGraph,
                                    propagation::SolverBase& solver) {
  if (invariantGraph.varId(outputVarNodeIds().front()) ==
      propagation::NULL_ID) {
    invariantGraph.varNode(outputVarNodeIds().front())
        .setVarId(solver.makeIntView<propagation::IntAbsView>(
            solver, invariantGraph.varId(input())));
  }
}

void IntAbsNode::registerNode(InvariantGraph&, propagation::SolverBase&) {}

}  // namespace atlantis::invariantgraph
