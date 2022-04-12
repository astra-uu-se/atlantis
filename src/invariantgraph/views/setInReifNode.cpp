#include "invariantgraph/views/setInReifNode.hpp"

#include "../parseHelper.hpp"
#include "invariantgraph/constraints/setInNode.hpp"
#include "invariantgraph/views/reifiedConstraint.hpp"
#include "utils/variant.hpp"

std::unique_ptr<invariantgraph::SetInReifNode>
invariantgraph::SetInReifNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "set_in_reif");
  assert(constraint.arguments.size() == 3);

  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto valueSet = integerSet(model, constraint.arguments[1]);
  auto r = mappedVariable(constraint.arguments[2], variableMap);

  return std::visit<std::unique_ptr<SetInReifNode>>(
      overloaded{
          [&](const fznparser::LiteralSet<Int>& set) {
            return std::make_unique<invariantgraph::SetInReifNode>(
                std::make_unique<invariantgraph::SetInNode>(a, set.values), r);
          },
          [&](const fznparser::IntRange& range) {
            std::vector<Int> values;
            values.resize(range.upperBound - range.lowerBound + 1);
            std::iota(values.begin(), values.end(), range.lowerBound);
            return std::make_unique<invariantgraph::SetInReifNode>(
                std::make_unique<invariantgraph::SetInNode>(a, values), r);
          }},
      valueSet);
}
