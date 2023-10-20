#include "invariantgraph/views/bool2IntNode.hpp"

#include "views/bool2IntView.hpp"

namespace invariantgraph {

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
                                           Engine& engine) {
  if (invariantGraph.varId(outputVarNodeIds().front()) == NULL_ID) {
    invariantGraph.varNode(outputVarNodeIds().front())
        .setVarId(engine.makeIntView<Bool2IntView>(
            engine, invariantGraph.varId(input())));
  }
}

void Bool2IntNode::registerNode(InvariantGraph&, Engine&) {}

}  // namespace invariantgraph