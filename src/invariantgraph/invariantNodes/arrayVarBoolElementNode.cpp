#include "invariantgraph/invariantNodes/arrayVarBoolElementNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<ArrayVarBoolElementNode>
ArrayVarBoolElementNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  const fznparser::IntArg b =
      std::get<fznparser::IntArg>(constraint.arguments().at(0));

  // Compute offset if nonshifted variant:
  Int offset = constraint.identifier() != "array_var_bool_element_nonshifted"
                   ? 1
                   : (b.isParameter() ? b.parameter() : b.var().lowerBound());

  const fznparser::BoolVarArray as =
      std::get<fznparser::BoolVarArray>(constraint.arguments().at(1));

  const fznparser::BoolArg c =
      std::get<fznparser::BoolArg>(constraint.arguments().at(2));

  return std::make_unique<ArrayVarBoolElementNode>(
      invariantGraph.createVarNode(b), invariantGraph.createVarNodes(as),
      invariantGraph.createVarNode(c), offset);
}

void ArrayVarBoolElementNode::registerOutputVariables(
    InvariantGraph& invariantGraph, Engine& engine) {
  // TODO: offset can be different than 1
  registerDefinedVariable(engine, outputVarNodeIds().front(), 1);
}

void ArrayVarBoolElementNode::registerNode(InvariantGraph& invariantGraph,
                                           Engine& engine) {
  std::vector<VarId> as;
  std::transform(dynamicInputVarNodeIds().begin(),
                 dynamicInputVarNodeIds().end(), std::back_inserter(as),
                 [&](auto node) { return node->varId(); });

  assert(outputVarNodeIds().front()->varId() != NULL_ID);
  engine.makeInvariant<ElementVar>(engine, outputVarNodeIds().front()->varId(),
                                   b()->varId(), as);
}

}  // namespace invariantgraph