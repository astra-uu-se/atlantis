#include "invariantgraph/invariantNodes/arrayIntMinimumNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<ArrayIntMinimumNode> ArrayIntMinimumNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  const fznparser::IntArg& output =
      std::get<fznparser::IntArg>(constraint.arguments().at(0));

  const fznparser::IntVarArray& inputs =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(1));

  return std::make_unique<ArrayIntMinimumNode>(
      invariantGraph.nextInvariantNodeId(),
      invariantGraph.createVarNodes(inputs),
      invariantGraph.createVarNode(output));
}

void ArrayIntMinimumNode::registerOutputVariables(
    InvariantGraph& invariantGraph, Engine& engine) {
  registerDefinedVariable(engine, outputVarNodeIds().front());
}

void ArrayIntMinimumNode::registerNode(InvariantGraph& invariantGraph,
                                       Engine& engine) {
  std::vector<VarId> variables;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(variables),
                 [&](const auto& node) { return node->varId(); });

  assert(outputVarNodeIds().front()->varId() != NULL_ID);
  engine.makeInvariant<MinSparse>(engine, outputVarNodeIds().front()->varId(),
                                  variables);
}

}  // namespace invariantgraph