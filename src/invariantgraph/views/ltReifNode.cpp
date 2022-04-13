#include "invariantgraph/views/ltReifNode.hpp"

#include "../parseHelper.hpp"
#include "invariantgraph/constraints/linLeNode.hpp"

std::unique_ptr<invariantgraph::LtReifNode>
invariantgraph::LtReifNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "int_lt_reif" || constraint.name == "bool_lt_reif");
  assert(constraint.arguments.size() == 3);

  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);
  auto r = mappedVariable(constraint.arguments[2], variableMap);

  return std::make_unique<invariantgraph::LtReifNode>(
      std::make_unique<invariantgraph::LinLeNode>(
          std::vector<Int>{1, -1}, std::vector<VariableNode*>{a, b}, -1),
      r);
}
