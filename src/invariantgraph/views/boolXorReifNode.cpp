#include "invariantgraph/views/boolXorReifNode.hpp"

#include "invariantgraph/constraints/linEqNode.hpp"
#include "invariantgraph/parseHelper.hpp"

std::unique_ptr<invariantgraph::BoolXorReifNode>
invariantgraph::BoolXorReifNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "bool_xor_reif");
  assert(constraint.arguments.size() == 3);

  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);
  auto r = mappedVariable(constraint.arguments[2], variableMap);

  return std::make_unique<BoolXorReifNode>(
      std::make_unique<LinEqNode>(std::vector<Int>{1, 1},
                                  std::vector<VariableNode*>{a, b}, 1),
      r);
}
