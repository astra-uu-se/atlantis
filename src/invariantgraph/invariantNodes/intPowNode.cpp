#include "atlantis/invariantgraph/invariantNodes/intPowNode.hpp"

#include <cmath>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/pow.hpp"

namespace atlantis::invariantgraph {

Int int_pow(Int base, Int power) {
  if (power == 0) {
    return 1;
  }
  if (power == 1) {
    return base;
  }
  if (power < 0) {
    if (base == 0) {
      throw std::runtime_error("negative power of zero");
    }
    if (base == 1) {
      return 1;
    }
    if (base == -1) {
      return power % 2 == 0 ? 1 : -1;
    }
    return 0;
  }
  Int result = 1;
  for (int i = 0; i < power; i++) {
    result *= base;
  }
  return result;
}

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
