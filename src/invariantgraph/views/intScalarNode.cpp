#include "atlantis/invariantgraph/views/intScalarNode.hpp"

#include "atlantis/propagation/views/scalarView.hpp"

namespace atlantis::invariantgraph {

IntScalarNode::IntScalarNode(VarNodeId staticInput, VarNodeId output,
                             Int factor, Int offset)
    : InvariantNode({output}, {staticInput}),
      _factor(factor),
      _offset(offset) {}

void IntScalarNode::updateState(InvariantGraph& graph) {
  if (_factor == 0) {
    graph.varNode(outputVarNodeIds().front()).fixToValue(_offset);
    setState(InvariantNodeState::SUBSUMED);
  } else if (graph.varNodeConst(staticInputVarNodeIds().front()).isFixed()) {
    graph.varNode(outputVarNodeIds().front())
        .fixToValue(_factor *
                        graph.varNodeConst(staticInputVarNodeIds().front())
                            .lowerBound() +
                    _offset);
    setState(InvariantNodeState::SUBSUMED);
  }
}

void IntScalarNode::registerOutputVars(InvariantGraph& invariantGraph,
                                       propagation::SolverBase& solver) {
  if (invariantGraph.varId(outputVarNodeIds().front()) ==
      propagation::NULL_ID) {
    invariantGraph.varNode(outputVarNodeIds().front())
        .setVarId(solver.makeIntView<propagation::ScalarView>(
            solver, invariantGraph.varId(input()), _factor, _offset));
  }
}

void IntScalarNode::registerNode(InvariantGraph&, propagation::SolverBase&) {}

}  // namespace atlantis::invariantgraph
