#include "invariantgraph/invariants/intLinLeReifNode.hpp"

#include "../parseHelper.hpp"
#include "invariantgraph/constraints/linLeNode.hpp"
#include "invariantgraph/invariants/reifiedConstraint.hpp"

std::unique_ptr<invariantgraph::IntLinLeReifNode>
invariantgraph::IntLinLeReifNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "int_lin_le_reif");
  assert(constraint.arguments.size() == 4);

  auto as = integerVector(model, constraint.arguments[0]);
  auto bs = mappedVariableVector(model, constraint.arguments[1], variableMap);
  auto c = integerValue(model, constraint.arguments[2]);
  auto r = mappedVariable(constraint.arguments[3], variableMap);

  return std::make_unique<invariantgraph::IntLinLeReifNode>(
      std::make_unique<invariantgraph::LinLeNode>(as, bs, c), r);
}
