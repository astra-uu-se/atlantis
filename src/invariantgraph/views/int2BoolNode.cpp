#include "atlantis/invariantgraph/views/int2BoolNode.hpp"

#include <map>
#include <utility>

#include "atlantis/propagation/views/int2BoolView.hpp"

namespace atlantis::invariantgraph {

Int2BoolNode::Int2BoolNode(InvariantGraph& graph, VarNodeId staticInput,
                           VarNodeId output)
    : InvariantNode(graph, {output}, {staticInput}) {}

void Int2BoolNode::init(const InvariantNodeId& id) {
  InvariantNode::init(id);
  assert(!graph.varNodeConst(outputVarNodeIds().front()).isIntVar());
  assert(graph.varNodeConst(staticInputVarNodeIds().front()).isIntVar());
}

void Int2BoolNode::updateState() {
  graph.varNode(input()).domain().removeBelow(Int{0});
  graph.varNode(input()).domain().removeAbove(Int{1});

  graph.varNode(outputVarNodeIds().front()).domain().removeBelow(Int{0});
  graph.varNode(outputVarNodeIds().front()).domain().removeAbove(Int{1});

  if (graph.varNodeConst(input()).isFixed()) {
    graph.varNode(outputVarNodeIds().front())
        .fixToValue(graph.varNodeConst(input()).inDomain(Int{1}));
    setState(InvariantNodeState::SUBSUMED);
  } else if (graph.varNodeConst(outputVarNodeIds().front()).isFixed()) {
    graph.varNode(input()).fixToValue(
        graph.varNodeConst(outputVarNodeIds().front()).inDomain(bool{true})
            ? Int{1}
            : Int{0});
    setState(InvariantNodeState::SUBSUMED);
  }
}

void Int2BoolNode::registerOutputVars() {
  if (graph.varId(outputVarNodeIds().front()) == propagation::NULL_ID) {
    graph.varNode(outputVarNodeIds().front())
        .setVarId(solver.makeIntView<propagation::Int2BoolView>(
            solver, graph.varId(input())));
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void Int2BoolNode::registerNode() {}

}  // namespace atlantis::invariantgraph
