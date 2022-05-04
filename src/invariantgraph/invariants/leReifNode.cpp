#include "invariantgraph/invariants/leReifNode.hpp"

#include "../parseHelper.hpp"
#include "invariantgraph/constraints/linLeNode.hpp"

std::unique_ptr<invariantgraph::LeReifNode>
invariantgraph::LeReifNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "int_le_reif" || constraint.name == "bool_le_reif");
  assert(constraint.arguments.size() == 3);

  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);
  auto r = mappedVariable(constraint.arguments[2], variableMap);

  return std::make_unique<invariantgraph::LeReifNode>(
      std::make_unique<invariantgraph::LinLeNode>(
          std::vector<Int>{1, -1}, std::vector<VariableNode*>{a, b}, 0),
      r);
}
