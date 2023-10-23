#include "invariantgraph/views/bool2IntNode.hpp"

#include "propagation/views/bool2IntView.hpp"

namespace atlantis::invariantgraph {

Bool2IntNode::Bool2IntNode(VarNodeId staticInput, VarNodeId output)
    : InvariantNode({output}, {staticInput}) {}

std::unique_ptr<Bool2IntNode> Bool2IntNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  const fznparser::BoolArg a =
      std::get<fznparser::BoolArg>(constraint.arguments().at(0));

  const fznparser::IntArg b =
      std::get<fznparser::IntArg>(constraint.arguments().at(1));

  return std::make_unique<Bool2IntNode>(invariantGraph.createVarNode(a),
                                        invariantGraph.createVarNode(b));
}

void Bool2IntNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                           propagation::SolverBase& solver) {
  if (invariantGraph.varId(outputVarNodeIds().front()) == propagation::NULL_ID) {
    invariantGraph.varNode(outputVarNodeIds().front())
        .setVarId(solver.makeIntView<propagation::Bool2IntView>(
            solver, invariantGraph.varId(input())));
  }
}

void Bool2IntNode::registerNode(InvariantGraph&, propagation::SolverBase&) {}

}  // namespace invariantgraph