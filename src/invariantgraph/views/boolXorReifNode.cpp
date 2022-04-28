#include "invariantgraph/views/boolXorReifNode.hpp"

#include "../parseHelper.hpp"
#include "invariantgraph/constraints/intNeNode.hpp"

std::unique_ptr<invariantgraph::BoolXorReifNode>
invariantgraph::BoolXorReifNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "bool_xor_reif");
  assert(constraint.arguments.size() == 3);

  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);
  auto r = mappedVariable(constraint.arguments[2], variableMap);

  return std::make_unique<BoolXorReifNode>(std::make_unique<IntNeNode>(a, b),
                                           r);
}
