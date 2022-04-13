#include "invariantgraph/views/intLinEqReifNode.hpp"

#include "../parseHelper.hpp"
#include "invariantgraph/constraints/linEqNode.hpp"
#include "invariantgraph/views/reifiedConstraint.hpp"

std::unique_ptr<invariantgraph::IntLinEqReifNode>
invariantgraph::IntLinEqReifNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "int_lin_eq_reif");
  assert(constraint.arguments.size() == 4);

  auto as = integerVector(model, constraint.arguments[0]);
  auto bs = mappedVariableVector(model, constraint.arguments[1], variableMap);
  auto c = integerValue(model, constraint.arguments[2]);
  auto r = mappedVariable(constraint.arguments[3], variableMap);

  return std::make_unique<invariantgraph::IntLinEqReifNode>(
      std::make_unique<invariantgraph::LinEqNode>(as, bs, c), r);
}
