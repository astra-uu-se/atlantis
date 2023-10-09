#include "invariantgraph/invariantNodes/arrayIntMinimumNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

ArrayIntMinimumNode::ArrayIntMinimumNode(std::vector<VarNodeId>&& variables,
                                         VarNodeId output)
    : InvariantNode({output}, std::move(variables)) {}

std::unique_ptr<ArrayIntMinimumNode> ArrayIntMinimumNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  const fznparser::IntArg& output =
      std::get<fznparser::IntArg>(constraint.arguments().at(0));

  const fznparser::IntVarArray& inputs =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(1));

  return std::make_unique<ArrayIntMinimumNode>(
      invariantGraph.createVarNodes(inputs),
      invariantGraph.createVarNode(output));
}

void ArrayIntMinimumNode::registerOutputVariables(
    InvariantGraph& invariantGraph, Engine& engine) {
  makeEngineVar(engine, invariantGraph.varNode(outputVarNodeIds().front()));
}

void ArrayIntMinimumNode::registerNode(InvariantGraph& invariantGraph,
                                       Engine& engine) {
  std::vector<VarId> variables;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(variables),
                 [&](const auto& node) { return invariantGraph.varId(node); });

  assert(invariantGraph.varId(outputVarNodeIds().front()) != NULL_ID);
  engine.makeInvariant<MinSparse>(
      engine, invariantGraph.varId(outputVarNodeIds().front()), variables);
}

}  // namespace invariantgraph