#include "atlantis/invariantgraph/invariantNodes/intDivNode.hpp"

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/intDiv.hpp"

namespace atlantis::invariantgraph {

IntDivNode::IntDivNode(VarNodeId nominator, VarNodeId denominator,
                       VarNodeId quotient)
    : InvariantNode({quotient}, {nominator, denominator}) {}

void invariantgraph::IntDivNode::registerOutputVars(
    InvariantGraph& invariantGraph, propagation::SolverBase& solver) {
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()));
}

void invariantgraph::IntDivNode::registerNode(InvariantGraph& invariantGraph,
                                              propagation::SolverBase& solver) {
  assert(invariantGraph.varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);
  solver.makeInvariant<propagation::IntDiv>(
      solver, invariantGraph.varId(outputVarNodeIds().front()),
      invariantGraph.varId(numerator()), invariantGraph.varId(denominator()));
}

VarNodeId IntDivNode::numerator() const noexcept {
  return staticInputVarNodeIds().front();
}
VarNodeId IntDivNode::denominator() const noexcept {
  return staticInputVarNodeIds().back();
}

}  // namespace atlantis::invariantgraph
