#include "invariantgraph/invariantNodes/intPowNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

IntPowNode::IntPowNode(VarNodeId base, VarNodeId exponent, VarNodeId power)
    : InvariantNode({power}, {base, exponent}) {}

void IntPowNode::registerOutputVars(InvariantGraph& invariantGraph,
                                    propagation::SolverBase& solver) {
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()));
}

void IntPowNode::registerNode(InvariantGraph& invariantGraph,
                              propagation::SolverBase& solver) {
  assert(invariantGraph.varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);
  solver.makeInvariant<propagation::Pow>(
      solver, invariantGraph.varId(outputVarNodeIds().front()),
      invariantGraph.varId(base()), invariantGraph.varId(exponent()));
}

}  // namespace atlantis::invariantgraph