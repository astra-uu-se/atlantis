#include "invariantgraph/invariantNodes/intPlusNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

IntPlusNode::IntPlusNode(VarNodeId a, VarNodeId b, VarNodeId output)
    : InvariantNode({output}, {a, b}) {}

std::unique_ptr<IntPlusNode> IntPlusNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  const fznparser::IntArg& a =
      std::get<fznparser::IntArg>(constraint.arguments().at(0));
  const fznparser::IntArg& b =
      std::get<fznparser::IntArg>(constraint.arguments().at(1));
  const fznparser::IntArg& output =
      std::get<fznparser::IntArg>(constraint.arguments().at(2));

  return std::make_unique<IntPlusNode>(invariantGraph.createVarNode(a),
                                       invariantGraph.createVarNode(b),
                                       invariantGraph.createVarNode(output));
}

void IntPlusNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                          Engine& engine) {
  makeEngineVar(engine, invariantGraph.varNode(outputVarNodeIds().front()));
}

void IntPlusNode::registerNode(InvariantGraph& invariantGraph, Engine& engine) {
  assert(invariantGraph.varId(outputVarNodeIds().front()) != NULL_ID);
  engine.makeInvariant<Plus>(
      engine, invariantGraph.varId(outputVarNodeIds().front()),
      invariantGraph.varId(a()), invariantGraph.varId(b()));
}

}  // namespace invariantgraph