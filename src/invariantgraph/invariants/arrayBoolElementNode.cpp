#include "invariantgraph/invariants/arrayBoolElementNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::ArrayIntElementNode>
invariantgraph::ArrayBoolElementNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "array_bool_element");
  assert(constraint.arguments.size() == 3);

  auto b = mappedVariable(constraint.arguments[0], variableMap);
  auto as = boolVectorAsIntVector(model, constraint.arguments[1]);
  auto r = mappedVariable(constraint.arguments[2], variableMap);

  return std::make_unique<invariantgraph::ArrayIntElementNode>(as, b, r);
}
