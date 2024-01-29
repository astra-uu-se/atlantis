#include "invariantgraph/views/intAbsNode.hpp"

namespace atlantis::invariantgraph {

IntAbsNode::IntAbsNode(VarNodeId staticInput, VarNodeId output)
    : InvariantNode({output}, {staticInput}) {}

void invariantgraph::IntAbsNode::registerOutputVars(
    InvariantGraph& invariantGraph, propagation::SolverBase& solver) {
  if (invariantGraph.varId(outputVarNodeIds().front()) ==
      propagation::NULL_ID) {
    invariantGraph.varNode(outputVarNodeIds().front())
        .setVarId(solver.makeIntView<propagation::IntAbsView>(
            solver, invariantGraph.varId(input())));
  }
}

void invariantgraph::IntAbsNode::registerNode(InvariantGraph&,
                                              propagation::SolverBase&) {}

}  // namespace atlantis::invariantgraph