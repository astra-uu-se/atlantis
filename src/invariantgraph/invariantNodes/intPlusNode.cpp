#include "invariantgraph/invariantNodes/intPlusNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

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
  registerDefinedVariable(engine, outputVarNodeIds().front());
}

void IntPlusNode::registerNode(InvariantGraph& invariantGraph, Engine& engine) {
  assert(outputVarNodeIds().front()->varId() != NULL_ID);
  engine.makeInvariant<Plus>(engine, outputVarNodeIds().front()->varId(),
                             a()->varId(), b()->varId());
}

}  // namespace invariantgraph