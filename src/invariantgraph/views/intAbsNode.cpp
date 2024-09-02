#include "atlantis/invariantgraph/views/intAbsNode.hpp"

#include <map>
#include <utility>

#include "atlantis/propagation/views/intAbsView.hpp"

namespace atlantis::invariantgraph {

IntAbsNode::IntAbsNode(VarNodeId staticInput, VarNodeId output)
    : InvariantNode({output}, {staticInput}) {}

void IntAbsNode::init(InvariantGraph& graph, const InvariantNodeId& id) {
  InvariantNode::init(graph, id);
  assert(graph.varNodeConst(outputVarNodeIds().front()).isIntVar());
  assert(graph.varNodeConst(staticInputVarNodeIds().front()).isIntVar());
}

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

void IntAbsNode::registerOutputVars(InvariantGraph& graph,
                                    propagation::SolverBase& solver) {
  if (graph.varId(outputVarNodeIds().front()) == propagation::NULL_ID) {
    graph.varNode(outputVarNodeIds().front())
        .setVarId(solver.makeIntView<propagation::IntAbsView>(
            solver, graph.varId(input())));
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void IntAbsNode::registerNode(InvariantGraph&, propagation::SolverBase&) {}

std::string IntAbsNode::dotLangIdentifier() const { return "abs"; }

}  // namespace atlantis::invariantgraph
