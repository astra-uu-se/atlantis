#include "invariantgraph/invariantNodes/intDivNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

IntDivNode::IntDivNode(VarNodeId a, VarNodeId b, VarNodeId output)
    : InvariantNode({output}, {a, b}) {}

std::unique_ptr<invariantgraph::IntDivNode>
invariantgraph::IntDivNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  const fznparser::IntArg& a =
      std::get<fznparser::IntArg>(constraint.arguments().at(0));
  const fznparser::IntArg& b =
      std::get<fznparser::IntArg>(constraint.arguments().at(1));
  const fznparser::IntArg& output =
      std::get<fznparser::IntArg>(constraint.arguments().at(2));

  return std::make_unique<IntDivNode>(invariantGraph.createVarNode(a),
                                      invariantGraph.createVarNode(b),
                                      invariantGraph.createVarNode(output));
}

void invariantgraph::IntDivNode::registerOutputVariables(
    InvariantGraph& invariantGraph, Engine& engine) {
  makeEngineVar(engine, invariantGraph.varNode(outputVarNodeIds().front()));
}

void invariantgraph::IntDivNode::registerNode(InvariantGraph& invariantGraph,
                                              Engine& engine) {
  assert(invariantGraph.varId(outputVarNodeIds().front()) != NULL_ID);
  engine.makeInvariant<IntDiv>(
      engine, invariantGraph.varId(outputVarNodeIds().front()),
      invariantGraph.varId(a()), invariantGraph.varId(b()));
}

VarNodeId IntDivNode::a() const noexcept {
  return staticInputVarNodeIds().front();
}
VarNodeId IntDivNode::b() const noexcept {
  return staticInputVarNodeIds().back();
}

}  // namespace invariantgraph