#include "invariantgraph/views/boolNotNode.hpp"

namespace invariantgraph {

BoolNotNode::BoolNotNode(VarNodeId staticInput, VarNodeId output)
    : InvariantNode({output}, {staticInput}) {}

std::unique_ptr<BoolNotNode> BoolNotNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  const fznparser::BoolArg a =
      std::get<fznparser::BoolArg>(constraint.arguments().at(0));
  const fznparser::BoolArg b =
      std::get<fznparser::BoolArg>(constraint.arguments().at(1));

  return std::make_unique<BoolNotNode>(invariantGraph.createVarNode(a),
                                       invariantGraph.createVarNode(b));
}

void BoolNotNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                          Engine& engine) {
  if (invariantGraph.varId(outputVarNodeIds().front()) == NULL_ID) {
    invariantGraph.varNode(outputVarNodeIds().front())
        .setVarId(engine.makeIntView<Bool2IntView>(
            engine, invariantGraph.varId(input())));
  }
}

void BoolNotNode::registerNode(InvariantGraph&, Engine&) {}

}  // namespace invariantgraph