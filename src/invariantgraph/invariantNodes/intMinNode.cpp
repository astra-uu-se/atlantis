#include "invariantgraph/invariantNodes/intMinNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

IntMinNode::IntMinNode(VarNodeId a, VarNodeId b, VarNodeId output)
    : InvariantNode({output}, {a, b}) {}

std::unique_ptr<IntMinNode> IntMinNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  const fznparser::IntArg& a =
      std::get<fznparser::IntArg>(constraint.arguments().at(0));
  const fznparser::IntArg& b =
      std::get<fznparser::IntArg>(constraint.arguments().at(1));
  const fznparser::IntArg& output =
      std::get<fznparser::IntArg>(constraint.arguments().at(2));

  return std::make_unique<IntMinNode>(invariantGraph.createVarNode(a),
                                      invariantGraph.createVarNode(b),
                                      invariantGraph.createVarNode(output));
}

void IntMinNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                         propagation::Engine& engine) {
  makeEngineVar(engine, invariantGraph.varNode(outputVarNodeIds().front()));
}

void IntMinNode::registerNode(InvariantGraph& invariantGraph, propagation::Engine& engine) {
  assert(invariantGraph.varId(outputVarNodeIds().front()) != propagation::NULL_ID);
  engine.makeInvariant<propagation::BinaryMin>(
      engine, invariantGraph.varId(outputVarNodeIds().front()),
      invariantGraph.varId(a()), invariantGraph.varId(b()));
}

}  // namespace invariantgraph