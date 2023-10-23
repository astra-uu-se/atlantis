#include "invariantgraph/invariantNodes/arrayIntMaximumNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

ArrayIntMaximumNode::ArrayIntMaximumNode(std::vector<VarNodeId>&& variables,
                                         VarNodeId output)
    : InvariantNode({output}, std::move(variables)) {}

std::unique_ptr<ArrayIntMaximumNode> ArrayIntMaximumNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  const fznparser::IntArg& output =
      std::get<fznparser::IntArg>(constraint.arguments().at(0));

  const fznparser::IntVarArray& inputs =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(1));

  return std::make_unique<ArrayIntMaximumNode>(
      invariantGraph.createVarNodes(inputs),
      invariantGraph.createVarNode(output));
}

void ArrayIntMaximumNode::registerOutputVariables(
    InvariantGraph& invariantGraph, propagation::Engine& engine) {
  makeEngineVar(engine, invariantGraph.varNode(outputVarNodeIds().front()));
}

void ArrayIntMaximumNode::registerNode(InvariantGraph& invariantGraph,
                                       propagation::Engine& engine) {
  std::vector<propagation::VarId> variables;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(variables), [&](const auto& node) {
                   return invariantGraph.varId(node);
                 });

  assert(invariantGraph.varId(outputVarNodeIds().front()) != propagation::NULL_ID);
  engine.makeInvariant<propagation::MaxSparse>(
      engine, invariantGraph.varId(outputVarNodeIds().front()),
      variables);
}

}  // namespace invariantgraph