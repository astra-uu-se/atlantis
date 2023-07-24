#include "invariantgraph/invariants/arrayBoolElementNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<ArrayIntElementNode> ArrayBoolElementNode::fromModelConstraint(
    const fznparser::Model&, const fznparser::Constraint& constraint,
    InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  const fznparser::IntVar& idx =
      std::get<std::reference_wrapper<const fznparser::IntVar>>(
          std::get<fznparser::IntArg>(constraint.arguments().at(0)));

  std::vector<Int> as = toIntVector(
      std::get<fznParser::BoolVarArray>(constraint.arguments().at(1))
          .toParVector());

  const fznparser::BoolVar& c =
      std::get<std::reference_wrapper<const fznparser::BoolVar>>(
          std::get<fznparser::BoolArg>(constraint.arguments().at(2)));

  const Int offset =
      constraint.name != "array_bool_element_offset" ? 1 : idx.lowerBound();

  return std::make_unique<ArrayIntElementNode>(
      as, invariantGraph.addVariable(idx), invariantGraph.addVariable(c),
      offset);
}

}  // namespace invariantgraph