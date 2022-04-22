#include "invariantgraph/views/eqReifNode.hpp"

#include "invariantgraph/constraints/eqNode.hpp"
#include "invariantgraph/parseHelper.hpp"

std::unique_ptr<invariantgraph::EqReifNode>
invariantgraph::EqReifNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "int_eq_reif" || constraint.name == "bool_eq_reif");
  assert(constraint.arguments.size() == 3);

  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);
  auto r = mappedVariable(constraint.arguments[2], variableMap);

  return std::make_unique<invariantgraph::EqReifNode>(
      std::make_unique<invariantgraph::EqNode>(a, b), r);
}
