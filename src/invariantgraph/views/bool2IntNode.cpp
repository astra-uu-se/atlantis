#include "atlantis/invariantgraph/views/bool2IntNode.hpp"

#include <map>
#include <utility>

#include "atlantis/propagation/views/bool2IntView.hpp"

namespace atlantis::invariantgraph {

Bool2IntNode::Bool2IntNode(VarNodeId staticInput, VarNodeId output)
    : InvariantNode({output}, {staticInput}) {}

void Bool2IntNode::updateState(InvariantGraph& graph) {
  graph.varNode(input()).domain().removeBelow(Int{0});
  graph.varNode(input()).domain().removeAbove(Int{1});

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

void Bool2IntNode::registerOutputVars(InvariantGraph& invariantGraph,
                                      propagation::SolverBase& solver) {
  if (invariantGraph.varId(outputVarNodeIds().front()) ==
      propagation::NULL_ID) {
    invariantGraph.varNode(outputVarNodeIds().front())
        .setVarId(solver.makeIntView<propagation::Bool2IntView>(
            solver, invariantGraph.varId(input())));
  }
}

void Bool2IntNode::registerNode(InvariantGraph&, propagation::SolverBase&) {}

}  // namespace atlantis::invariantgraph
