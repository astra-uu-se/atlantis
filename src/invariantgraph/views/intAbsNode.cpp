#include "invariantgraph/views/intAbsNode.hpp"

namespace atlantis::invariantgraph {

IntAbsNode::IntAbsNode(VarNodeId staticInput, VarNodeId output)
    : InvariantNode({output}, {staticInput}) {}

std::unique_ptr<invariantgraph::IntAbsNode>
invariantgraph::IntAbsNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  const fznparser::IntArg a =
      std::get<fznparser::IntArg>(constraint.arguments().at(0));
  const fznparser::IntArg b =
      std::get<fznparser::IntArg>(constraint.arguments().at(1));

  return std::make_unique<invariantgraph::IntAbsNode>(
      invariantGraph.createVarNode(a), invariantGraph.createVarNode(b));
}

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