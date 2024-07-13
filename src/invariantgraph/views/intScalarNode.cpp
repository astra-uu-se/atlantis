#include "atlantis/invariantgraph/views/intScalarNode.hpp"

#include "atlantis/propagation/views/scalarView.hpp"

namespace atlantis::invariantgraph {

IntScalarNode::IntScalarNode(VarNodeId staticInput, VarNodeId output,
                             Int factor, Int offset)
    : InvariantNode({output}, {staticInput}),
      _factor(factor),
      _offset(offset) {}

void IntScalarNode::init(InvariantGraph& graph, const InvariantNodeId& id) {
  InvariantNode::init(graph, id);
  assert(graph.varNodeConst(outputVarNodeIds().front()).isIntVar());
  assert(graph.varNodeConst(staticInputVarNodeIds().front()).isIntVar());
}

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

void IntScalarNode::registerOutputVars(InvariantGraph& graph,
                                       propagation::SolverBase& solver) {
  if (graph.varId(outputVarNodeIds().front()) == propagation::NULL_ID) {
    graph.varNode(outputVarNodeIds().front())
        .setVarId(solver.makeIntView<propagation::ScalarView>(
            solver, graph.varId(input()), _factor, _offset));
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void IntScalarNode::registerNode(InvariantGraph&, propagation::SolverBase&) {}

}  // namespace atlantis::invariantgraph
