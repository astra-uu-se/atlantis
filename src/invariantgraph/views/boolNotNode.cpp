#include "invariantgraph/views/boolNotNode.hpp"

namespace atlantis::invariantgraph {

BoolNotNode::BoolNotNode(VarNodeId staticInput, VarNodeId output)
    : InvariantNode({output}, {staticInput}) {}

std::unique_ptr<BoolNotNode> BoolNotNode::fromModelConstraint(
    const fznparser::Constraint& constraint, FznInvariantGraph& invariantGraph) {
  const fznparser::BoolArg a =
      std::get<fznparser::BoolArg>(constraint.arguments().at(0));
  const fznparser::BoolArg b =
      std::get<fznparser::BoolArg>(constraint.arguments().at(1));

  return std::make_unique<BoolNotNode>(invariantGraph.createVarNode(a),
                                       invariantGraph.createVarNode(b));
}

void BoolNotNode::registerOutputVars(InvariantGraph& invariantGraph,
                                          propagation::SolverBase& solver) {
  if (invariantGraph.varId(outputVarNodeIds().front()) == propagation::NULL_ID) {
    invariantGraph.varNode(outputVarNodeIds().front())
        .setVarId(solver.makeIntView<propagation::Bool2IntView>(
            solver, invariantGraph.varId(input())));
  }
}

void BoolNotNode::registerNode(InvariantGraph&, propagation::SolverBase&) {}

}  // namespace invariantgraph