#include "atlantis/invariantgraph/invariantNodes/intModNode.hpp"

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/mod.hpp"

namespace atlantis::invariantgraph {

IntModNode::IntModNode(VarNodeId numerator, VarNodeId denominator,
                       VarNodeId remainder)
    : InvariantNode({remainder}, {numerator, denominator}) {}

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

}  // namespace atlantis::invariantgraph
