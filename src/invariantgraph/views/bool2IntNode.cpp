#include "invariantgraph/views/bool2IntNode.hpp"

namespace atlantis::invariantgraph {

Bool2IntNode::Bool2IntNode(VarNodeId staticInput, VarNodeId output)
    : InvariantNode({output}, {staticInput}) {}

void Bool2IntNode::registerOutputVars(InvariantGraph& invariantGraph,
                                      propagation::SolverBase& solver) {
  if (invariantGraph.varId(outputVarNodeIds().front()) ==
      propagation::NULL_ID) {
    invariantGraph.varNode(outputVarNodeIds().front())
        .setVarId(solver.makeIntView<propagation::Bool2IntView>(
            solver, invariantGraph.varId(input())));
  }
}

void Bool2IntNode::registerNode(InvariantGraph&, propagation::SolverBase&) {}

}  // namespace atlantis::invariantgraph