#include "atlantis/invariantgraph/views/intModViewNode.hpp"

#include "atlantis/propagation/views/modView.hpp"

namespace atlantis::invariantgraph {

IntModViewNode::IntModViewNode(VarNodeId staticInput, VarNodeId output,
                               Int denominator)
    : InvariantNode({output}, {staticInput}),
      _denominator(std::abs(denominator)) {}

void IntModViewNode::init(InvariantGraph& graph, const InvariantNodeId& id) {
  InvariantNode::init(graph, id);
  assert(graph.varNodeConst(outputVarNodeIds().front()).isIntVar());
  assert(graph.varNodeConst(staticInputVarNodeIds().front()).isIntVar());
}

void IntModViewNode::updateState(InvariantGraph& graph) {
  if (graph.varNodeConst(staticInputVarNodeIds().front()).isFixed()) {
    graph.varNode(outputVarNodeIds().front())
        .fixToValue(
            graph.varNodeConst(staticInputVarNodeIds().front()).lowerBound() %
            _denominator);
    setState(InvariantNodeState::SUBSUMED);
  }
}

void IntModViewNode::registerOutputVars(InvariantGraph& graph,
                                        propagation::SolverBase& solver) {
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

void IntModViewNode::registerNode(InvariantGraph&, propagation::SolverBase&) {}

std::string IntModViewNode::dotLangIdentifier() const {
  return "% " + std::to_string(_denominator);
}

}  // namespace atlantis::invariantgraph
