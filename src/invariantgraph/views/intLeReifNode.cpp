#include "invariantgraph/views/intLeReifNode.hpp"

#include "../parseHelper.hpp"
#include "invariantgraph/constraints/eqNode.hpp"
#include "invariantgraph/views/reifiedConstraint.hpp"

std::unique_ptr<invariantgraph::IntLeReifNode>
invariantgraph::IntLeReifNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "int_le_reif");
  assert(constraint.arguments.size() == 3);

  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);
  auto r = mappedVariable(constraint.arguments[2], variableMap);

  return std::make_unique<invariantgraph::IntLeReifNode>(
      std::make_unique<invariantgraph::EqNode>(a, b), r);
}
