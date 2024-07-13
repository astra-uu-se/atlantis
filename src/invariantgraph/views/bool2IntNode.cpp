#include "atlantis/invariantgraph/views/bool2IntNode.hpp"

#include <map>
#include <utility>

#include "atlantis/propagation/views/bool2IntView.hpp"

namespace atlantis::invariantgraph {

Bool2IntNode::Bool2IntNode(VarNodeId staticInput, VarNodeId output)
    : InvariantNode({output}, {staticInput}) {}

void Bool2IntNode::init(InvariantGraph& graph, const InvariantNodeId& id) {
  InvariantNode::init(graph, id);
  assert(graph.varNodeConst(outputVarNodeIds().front()).isIntVar());
  assert(!graph.varNodeConst(staticInputVarNodeIds().front()).isIntVar());
}

void Bool2IntNode::updateState(InvariantGraph& graph) {
  graph.varNode(input()).domain().removeBelow(Int{0});
  graph.varNode(input()).domain().removeAbove(Int{1});

  graph.varNode(outputVarNodeIds().front()).domain().removeBelow(Int{0});
  graph.varNode(outputVarNodeIds().front()).domain().removeAbove(Int{1});

  if (graph.varNodeConst(input()).isFixed()) {
    graph.varNode(outputVarNodeIds().front())
        .fixToValue(graph.varNodeConst(input()).inDomain(bool{true}) ? Int{1}
                                                                     : Int{0});
    setState(InvariantNodeState::SUBSUMED);
  } else if (graph.varNodeConst(outputVarNodeIds().front()).isFixed()) {
    graph.varNode(input()).fixToValue(
        graph.varNodeConst(outputVarNodeIds().front()).inDomain(Int{1}));
    setState(InvariantNodeState::SUBSUMED);
  }
}

void Bool2IntNode::registerOutputVars(InvariantGraph& graph,
                                      propagation::SolverBase& solver) {
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

void Bool2IntNode::registerNode(InvariantGraph&, propagation::SolverBase&) {}

}  // namespace atlantis::invariantgraph
