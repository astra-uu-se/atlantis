#include "atlantis/invariantgraph/invariantNodes/intDivNode.hpp"

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/intDiv.hpp"

namespace atlantis::invariantgraph {

IntDivNode::IntDivNode(VarNodeId numerator, VarNodeId denominator,
                       VarNodeId quotient)
    : InvariantNode({quotient}, {numerator, denominator}) {}

bool IntDivNode::canBeReplaced(const InvariantGraph& graph) const {
  return graph.varNodeConst(denominator()).isFixed() &&
         graph.varNodeConst(denominator()).lowerBound() == 1;
}

bool IntDivNode::replace(InvariantGraph& invariantGraph) {
  if (!canBeReplaced(invariantGraph)) {
    return false;
  }
  assert(invariantGraph.varNode(denominator()).isFixed() &&
         invariantGraph.varNode(denominator()).lowerBound() == 1);
  invariantGraph.replaceVarNode(quotient(), numerator());
  return true;
}

void invariantgraph::IntDivNode::registerOutputVars(
    InvariantGraph& invariantGraph, propagation::SolverBase& solver) {
  makeSolverVar(solver, invariantGraph.varNode(quotient()));
}

void invariantgraph::IntDivNode::registerNode(InvariantGraph& invariantGraph,
                                              propagation::SolverBase& solver) {
  assert(invariantGraph.varId(quotient()) != propagation::NULL_ID);
  solver.makeInvariant<propagation::IntDiv>(
      solver, invariantGraph.varId(quotient()),
      invariantGraph.varId(numerator()), invariantGraph.varId(denominator()));
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
