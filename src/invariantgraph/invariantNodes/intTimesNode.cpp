#include "atlantis/invariantgraph/invariantNodes/intTimesNode.hpp"

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/times.hpp"

namespace atlantis::invariantgraph {

IntTimesNode::IntTimesNode(VarNodeId a, VarNodeId b, VarNodeId output)
    : InvariantNode({output}, {a, b}) {}

void IntTimesNode::registerOutputVars(InvariantGraph& invariantGraph,
                                      propagation::SolverBase& solver) {
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()));
}

void IntTimesNode::registerNode(InvariantGraph& invariantGraph,
                                propagation::SolverBase& solver) {
  assert(invariantGraph.varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);
  solver.makeInvariant<propagation::Times>(
      solver, invariantGraph.varId(outputVarNodeIds().front()),
      invariantGraph.varId(a()), invariantGraph.varId(b()));
}

VarNodeId IntTimesNode::a() const { return staticInputVarNodeIds().front(); }
VarNodeId IntTimesNode::b() const { return staticInputVarNodeIds().back(); }

}  // namespace atlantis::invariantgraph
