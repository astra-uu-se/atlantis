#include "atlantis/invariantgraph/views/intModViewNode.hpp"

#include "atlantis/propagation/views/modView.hpp"

namespace atlantis::invariantgraph {

IntModViewNode::IntModViewNode(VarNodeId staticInput, VarNodeId output,
                               Int denominator)
    : InvariantNode({output}, {staticInput}),
      _denominator(std::abs(denominator)) {}

void IntModViewNode::updateState(InvariantGraph& graph) {
  if (graph.varNodeConst(staticInputVarNodeIds().front()).isFixed()) {
    graph.varNode(outputVarNodeIds().front())
        .fixToValue(
            graph.varNodeConst(staticInputVarNodeIds().front()).lowerBound() %
            _denominator);
    setState(InvariantNodeState::SUBSUMED);
  }
}

void IntModViewNode::registerOutputVars(InvariantGraph& invariantGraph,
                                        propagation::SolverBase& solver) {
  if (invariantGraph.varId(outputVarNodeIds().front()) ==
      propagation::NULL_ID) {
    invariantGraph.varNode(outputVarNodeIds().front())
        .setVarId(solver.makeIntView<propagation::ModView>(
            solver, invariantGraph.varId(input()), _denominator));
  }
}

void IntModViewNode::registerNode(InvariantGraph&, propagation::SolverBase&) {}

}  // namespace atlantis::invariantgraph
