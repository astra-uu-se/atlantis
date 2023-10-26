#include "invariantgraph/invariantNodes/arrayBoolElementNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

std::unique_ptr<ArrayIntElementNode> ArrayBoolElementNode::fromModelConstraint(
    const fznparser::Constraint& constraint, FznInvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  const fznparser::IntArg& idx =
      std::get<fznparser::IntArg>(constraint.arguments().at(0));

  std::vector<Int> as = toIntVector(
      std::get<fznparser::BoolVarArray>(constraint.arguments().at(1))
          .toParVector());

  const fznparser::BoolArg& c =
      std::get<fznparser::BoolArg>(constraint.arguments().at(2));

  const Int offset =
      constraint.identifier() != "array_bool_element_offset"
          ? 1
          : (idx.isParameter() ? idx.parameter() : idx.var().lowerBound());

  return std::make_unique<ArrayIntElementNode>(
      std::move(as), invariantGraph.createVarNode(idx),
      invariantGraph.createVarNode(c), offset);
}

}  // namespace invariantgraph