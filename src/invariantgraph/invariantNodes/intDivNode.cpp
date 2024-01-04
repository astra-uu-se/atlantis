#include "invariantgraph/invariantNodes/intDivNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

IntDivNode::IntDivNode(VarNodeId _nominator, VarNodeId _denominator,
                       VarNodeId quotient)
    : InvariantNode({quotient}, {_nominator, _denominator}) {}

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
      invariantGraph.varId(nominator()), invariantGraph.varId(denominator()));
}

VarNodeId IntDivNode::nominator() const noexcept {
  return staticInputVarNodeIds().front();
}
VarNodeId IntDivNode::denominator() const noexcept {
  return staticInputVarNodeIds().back();
}

}  // namespace atlantis::invariantgraph