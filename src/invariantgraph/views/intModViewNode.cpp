#include "atlantis/invariantgraph/views/intModViewNode.hpp"

#include "atlantis/propagation/views/modView.hpp"

namespace atlantis::invariantgraph {

IntModViewNode::IntModViewNode(InvariantGraph& graph, VarNodeId staticInput,
                               VarNodeId output, Int denominator)
    : InvariantNode(graph, {output}, {staticInput}),
      _denominator(std::abs(denominator)) {}

void IntModViewNode::init(const InvariantNodeId& id) {
  InvariantNode::init(id);
  assert(graph.varNodeConst(outputVarNodeIds().front()).isIntVar());
  assert(graph.varNodeConst(staticInputVarNodeIds().front()).isIntVar());
}

void IntModViewNode::updateState() {
  if (graph.varNodeConst(staticInputVarNodeIds().front()).isFixed()) {
    graph.varNode(outputVarNodeIds().front())
        .fixToValue(
            graph.varNodeConst(staticInputVarNodeIds().front()).lowerBound() %
            _denominator);
    setState(InvariantNodeState::SUBSUMED);
  }
}

void IntModViewNode::registerOutputVars() {
  if (graph.varId(outputVarNodeIds().front()) == propagation::NULL_ID) {
    graph.varNode(outputVarNodeIds().front())
        .setVarId(solver.makeIntView<propagation::ModView>(
            solver, graph.varId(input()), _denominator));
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void IntModViewNode::registerNode() {}

}  // namespace atlantis::invariantgraph
