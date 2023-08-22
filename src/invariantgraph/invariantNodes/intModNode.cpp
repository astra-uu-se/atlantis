#include "invariantgraph/invariantNodes/intModNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<IntModNode> IntModNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  const fznparser::IntArg& a =
      std::get<fznparser::IntArg>(constraint.arguments().at(0));
  const fznparser::IntArg& b =
      std::get<fznparser::IntArg>(constraint.arguments().at(1));
  const fznparser::IntArg& output =
      std::get<fznparser::IntArg>(constraint.arguments().at(2));

  return std::make_unique<IntModNode>(invariantGraph.createVarNode(a),
                                      invariantGraph.createVarNode(b),
                                      invariantGraph.createVarNode(output));
}

void IntModNode::registerOutputVariables(InvariantGraph& invariantGraph,
                                         Engine& engine) {
  registerDefinedVariable(engine, outputVarNodeIds().front());
}

void IntModNode::registerNode(InvariantGraph& invariantGraph, Engine& engine) {
  assert(outputVarNodeIds().front()->varId() != NULL_ID);
  engine.makeInvariant<Mod>(engine, outputVarNodeIds().front()->varId(),
                            a()->varId(), b()->varId());
}

}  // namespace invariantgraph