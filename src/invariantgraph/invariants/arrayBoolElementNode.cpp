#include "invariantgraph/invariants/arrayBoolElementNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::ArrayIntElementNode>
invariantgraph::ArrayBoolElementNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto idx = mappedVariable(constraint.arguments[0], variableMap);
  auto as = boolVectorAsIntVector(model, constraint.arguments[1]);
  auto c = mappedVariable(constraint.arguments[2], variableMap);
  const Int offset = constraint.name != "array_bool_element_offset"
                         ? 1
                         : idx->domain().lowerBound();

  return std::make_unique<ArrayIntElementNode>(as, idx, c, offset);
}
