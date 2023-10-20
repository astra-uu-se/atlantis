#include "invariantgraph/invariantNodes/intTimesNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

IntTimesNode::IntTimesNode(VarNodeId a, VarNodeId b, VarNodeId output)
    : InvariantNode({output}, {a, b}) {}

std::unique_ptr<IntTimesNode> IntTimesNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  const fznparser::IntArg& a =
      std::get<fznparser::IntArg>(constraint.arguments().at(0));
  const fznparser::IntArg& b =
      std::get<fznparser::IntArg>(constraint.arguments().at(1));
  const fznparser::IntArg& output =
      std::get<fznparser::IntArg>(constraint.arguments().at(2));

  return std::make_unique<IntTimesNode>(invariantGraph.createVarNode(a),
                                        invariantGraph.createVarNode(b),
                                        invariantGraph.createVarNode(output));
}

void IntTimesNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                           Engine& engine) {
  makeEngineVar(engine, invariantGraph.varNode(outputVarNodeIds().front()));
}

void IntTimesNode::registerNode(InvariantGraph& invariantGraph,
                                Engine& engine) {
  assert(invariantGraph.varId(outputVarNodeIds().front()) != NULL_ID);
  engine.makeInvariant<Times>(
      engine, invariantGraph.varId(outputVarNodeIds().front()),
      invariantGraph.varId(a()), invariantGraph.varId(b()));
}

}  // namespace invariantgraph