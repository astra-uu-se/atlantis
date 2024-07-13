#include "atlantis/invariantgraph/invariantNodes/intModNode.hpp"

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/mod.hpp"

namespace atlantis::invariantgraph {

IntModNode::IntModNode(VarNodeId numerator, VarNodeId denominator,
                       VarNodeId remainder)
    : InvariantNode({remainder}, {numerator, denominator}) {}

void IntModNode::init(InvariantGraph& graph, const InvariantNodeId& id) {
  InvariantNode::init(graph, id);
  assert(graph.varNodeConst(remainder()).isIntVar());
  assert(graph.varNodeConst(numerator()).isIntVar());
  assert(graph.varNodeConst(denominator()).isIntVar());
}

void IntModNode::registerOutputVars(InvariantGraph& invariantGraph,
                                    propagation::SolverBase& solver) {
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()));
}

void IntModNode::registerNode(InvariantGraph& invariantGraph,
                              propagation::SolverBase& solver) {
  assert(invariantGraph.varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);
  solver.makeInvariant<propagation::Mod>(
      solver, invariantGraph.varId(outputVarNodeIds().front()),
      invariantGraph.varId(numerator()), invariantGraph.varId(denominator()));
}

VarNodeId IntModNode::numerator() const {
  return staticInputVarNodeIds().front();
}
VarNodeId IntModNode::denominator() const {
  return staticInputVarNodeIds().back();
}
VarNodeId IntModNode::remainder() const { return outputVarNodeIds().front(); }

}  // namespace atlantis::invariantgraph
