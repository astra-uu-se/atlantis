#include "atlantis/invariantgraph/invariantNodes/intPowNode.hpp"

#include <cmath>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/pow.hpp"

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

VarNodeId IntPowNode::base() const { return staticInputVarNodeIds().front(); }
VarNodeId IntPowNode::exponent() const {
  return staticInputVarNodeIds().back();
}
VarNodeId IntPowNode::power() const { return outputVarNodeIds().front(); }

}  // namespace atlantis::invariantgraph
