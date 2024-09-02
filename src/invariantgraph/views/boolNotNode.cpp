#include "atlantis/invariantgraph/views/boolNotNode.hpp"

#include "atlantis/propagation/views/bool2IntView.hpp"

namespace atlantis::invariantgraph {

BoolNotNode::BoolNotNode(InvariantGraph& graph, VarNodeId staticInput,
                         VarNodeId output)
    : InvariantNode(graph, {output}, {staticInput}) {}

void BoolNotNode::init(const InvariantNodeId& id) {
  InvariantNode::init(id);
  assert(!graph.varNodeConst(outputVarNodeIds().front()).isIntVar());
  assert(!graph.varNodeConst(staticInputVarNodeIds().front()).isIntVar());
}

void BoolNotNode::updateState() {
  if (graph.varNodeConst(input()).isFixed()) {
    graph.varNode(outputVarNodeIds().front())
        .fixToValue(!graph.varNodeConst(input()).inDomain(bool{true}));
    setState(InvariantNodeState::SUBSUMED);
  } else if (graph.varNodeConst(outputVarNodeIds().front()).isFixed()) {
    graph.varNode(input()).fixToValue(
        !graph.varNodeConst(outputVarNodeIds().front()).inDomain(bool{true}));
    setState(InvariantNodeState::SUBSUMED);
  }
}

void BoolNotNode::registerOutputVars() {
  if (graph.varId(outputVarNodeIds().front()) == propagation::NULL_ID) {
    graph.varNode(outputVarNodeIds().front())
        .setVarId(solver.makeIntView<propagation::Bool2IntView>(
            solver, graph.varId(input())));
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void BoolNotNode::registerNode() {}

}  // namespace atlantis::invariantgraph
