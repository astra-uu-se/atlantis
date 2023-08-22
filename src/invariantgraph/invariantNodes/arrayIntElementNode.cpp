#include "invariantgraph/invariantNodes/arrayIntElementNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<ArrayIntElementNode> ArrayIntElementNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  const fznparser::IntArg& idx =
      std::get<fznparser::IntArg>(constraint.arguments().at(0));

  std::vector<Int> as =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(1))
          .toParVector();

  const fznparser::IntArg& c =
      std::get<fznparser::IntArg>(constraint.arguments().at(2));

  const Int offset =
      constraint.identifier() != "array_int_element_offset"
          ? 1
          : (idx.isParameter() ? idx.parameter() : idx.var().lowerBound());

  return std::make_unique<ArrayIntElementNode>(
      std::move(as), invariantGraph.createVarNode(idx),
      invariantGraph.createVarNode(c), offset);
}

void ArrayIntElementNode::registerOutputVariables(
    InvariantGraph& invariantGraph, Engine& engine) {
  if (outputVarNodeIds().front()->varId() == NULL_ID) {
    assert(b()->varId() != NULL_ID);
    outputVarNodeIds().front()->setVarId(
        engine.makeIntView<ElementConst>(engine, b()->varId(), _as, _offset));
  }
}

void ArrayIntElementNode::registerNode(const InvariantGraph& invariantGraph,
                                       Engine&) {}

}  // namespace invariantgraph