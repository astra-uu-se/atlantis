#include "invariantgraph/views/boolNotNode.hpp"

std::unique_ptr<invariantgraph::BoolNotNode>
invariantgraph::BoolNotNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  const fznparser::BoolArg a =
      std::get<fznparser::BoolArg>(constraint.arguments().at(0));
  const fznparser::BoolArg b =
      std::get<fznparser::BoolArg>(constraint.arguments().at(1));

  return std::make_unique<invariantgraph::BoolNotNode>(
      invariantGraph.createVarNode(a), invariantGraph.createVarNode(b));
}

void invariantgraph::BoolNotNode::registerOutputVariables(
    InvariantGraph& invariantGraph, Engine& engine) {
  if (outputVarNodeIds().front()->varId() == NULL_ID) {
    outputVarNodeIds().front()->setVarId(
        engine.makeIntView<Bool2IntView>(engine, input()->varId()));
  }
}

void invariantgraph::BoolNotNode::registerNode(
    const InvariantGraph& invariantGraph, Engine&) {}
