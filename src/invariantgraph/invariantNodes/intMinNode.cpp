#include "invariantgraph/invariantNodes/intMinNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

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
                                         Engine& engine) {
  registerDefinedVariable(engine, outputVarNodeIds().front());
}

void IntMinNode::registerNode(InvariantGraph& invariantGraph, Engine& engine) {
  assert(outputVarNodeIds().front()->varId() != NULL_ID);
  engine.makeInvariant<BinaryMin>(engine, outputVarNodeIds().front()->varId(),
                                  a()->varId(), b()->varId());
}

}  // namespace invariantgraph