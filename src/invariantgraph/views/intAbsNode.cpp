#include "invariantgraph/views/intAbsNode.hpp"

#include "views/intAbsView.hpp"

std::unique_ptr<invariantgraph::IntAbsNode>
invariantgraph::IntAbsNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  const fznparser::IntArg a =
      std::get<fznparser::IntArg>(constraint.arguments().at(0));
  const fznparser::IntArg b =
      std::get<fznparser::IntArg>(constraint.arguments().at(1));

  return std::make_unique<invariantgraph::IntAbsNode>(
      invariantGraph.createVarNode(a), invariantGraph.createVarNode(b));
}

void invariantgraph::IntAbsNode::registerOutputVariables(
    InvariantGraph& invariantGraph, Engine& engine) {
  if (outputVarNodeIds().front()->varId() == NULL_ID) {
    outputVarNodeIds().front()->setVarId(
        engine.makeIntView<IntAbsView>(engine, input()->varId()));
  }
}

void invariantgraph::IntAbsNode::registerNode(
    const InvariantGraph& invariantGraph, Engine&) {}
