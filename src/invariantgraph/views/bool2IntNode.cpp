#include "invariantgraph/views/bool2IntNode.hpp"

#include "views/bool2IntView.hpp"

std::unique_ptr<invariantgraph::Bool2IntNode>
invariantgraph::Bool2IntNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  const fznparser::BoolArg a =
      std::get<fznparser::BoolArg>(constraint.arguments().at(0));

  const fznparser::IntArg b =
      std::get<fznparser::IntArg>(constraint.arguments().at(1));

  return std::make_unique<invariantgraph::Bool2IntNode>(
      invariantGraph.createVarNode(a), invariantGraph.createVarNode(b));
}

void invariantgraph::Bool2IntNode::registerOutputVariables(
    InvariantGraph& invariantGraph, Engine& engine) {
  if (outputVarNodeIds().front()->varId() == NULL_ID) {
    outputVarNodeIds().front()->setVarId(
        engine.makeIntView<Bool2IntView>(engine, input()->varId()));
  }
}

void invariantgraph::Bool2IntNode::registerNode(
    const InvariantGraph& invariantGraph, Engine&) {}
