#include "atlantis/invariantgraph/views/int2BoolNode.hpp"

#include <map>
#include <utility>

#include "atlantis/propagation/views/int2BoolView.hpp"

namespace atlantis::invariantgraph {

Int2BoolNode::Int2BoolNode(VarNodeId staticInput, VarNodeId output)
    : InvariantNode({output}, {staticInput}) {}

void Int2BoolNode::updateState(InvariantGraph& graph) {
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

void Int2BoolNode::registerOutputVars(InvariantGraph& invariantGraph,
                                      propagation::SolverBase& solver) {
  if (invariantGraph.varId(outputVarNodeIds().front()) ==
      propagation::NULL_ID) {
    invariantGraph.varNode(outputVarNodeIds().front())
        .setVarId(solver.makeIntView<propagation::Int2BoolView>(
            solver, invariantGraph.varId(input())));
  }
}

void Int2BoolNode::registerNode(InvariantGraph&, propagation::SolverBase&) {}

}  // namespace atlantis::invariantgraph
