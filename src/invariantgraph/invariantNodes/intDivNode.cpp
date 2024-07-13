#include "atlantis/invariantgraph/invariantNodes/intDivNode.hpp"

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/intDiv.hpp"

namespace atlantis::invariantgraph {

IntDivNode::IntDivNode(VarNodeId numerator, VarNodeId denominator,
                       VarNodeId quotient)
    : InvariantNode({quotient}, {numerator, denominator}) {}

void IntDivNode::init(InvariantGraph& graph, const InvariantNodeId& id) {
  InvariantNode::init(graph, id);
  assert(graph.varNodeConst(quotient()).isIntVar());
  assert(graph.varNodeConst(numerator()).isIntVar());
  assert(graph.varNodeConst(denominator()).isIntVar());
}

bool IntDivNode::canBeReplaced(const InvariantGraph& graph) const {
  return state() == InvariantNodeState::ACTIVE &&
         graph.varNodeConst(denominator()).isFixed() &&
         graph.varNodeConst(denominator()).lowerBound() == 1;
}

bool IntDivNode::replace(InvariantGraph& graph) {
  if (!canBeReplaced(graph)) {
    return false;
  }
  assert(graph.varNode(denominator()).isFixed() &&
         graph.varNode(denominator()).lowerBound() == 1);
  graph.replaceVarNode(quotient(), numerator());
  return true;
}

void invariantgraph::IntDivNode::registerOutputVars(
    InvariantGraph& graph, propagation::SolverBase& solver) {
  makeSolverVar(solver, graph.varNode(quotient()));
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void invariantgraph::IntDivNode::registerNode(InvariantGraph& graph,
                                              propagation::SolverBase& solver) {
  assert(graph.varId(quotient()) != propagation::NULL_ID);
  solver.makeInvariant<propagation::IntDiv>(solver, graph.varId(quotient()),
                                            graph.varId(numerator()),
                                            graph.varId(denominator()));
}

VarNodeId IntDivNode::numerator() const noexcept {
  return staticInputVarNodeIds().front();
}
VarNodeId IntDivNode::denominator() const noexcept {
  return staticInputVarNodeIds().back();
}
VarNodeId IntDivNode::quotient() const noexcept {
  return outputVarNodeIds().front();
}

}  // namespace atlantis::invariantgraph
