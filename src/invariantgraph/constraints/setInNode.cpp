#include "invariantgraph/constraints/setInNode.hpp"

#include "../parseHelper.hpp"
#include "constraints/inDomain.hpp"
#include "utils/variant.hpp"

std::unique_ptr<invariantgraph::SetInNode>
invariantgraph::SetInNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "set_in");
  assert(constraint.arguments.size() == 2);

  auto variable = mappedVariable(constraint.arguments[0], variableMap);
  auto valueSet = integerSet(model, constraint.arguments[1]);

  // Note: if the valueSet is an IntRange, here all the values are collected
  // into a vector. If it turns out memory usage is an issue, this should be
  // mitigated.

  return std::visit<std::unique_ptr<SetInNode>>(
      overloaded{[&](const fznparser::LiteralSet<Int>& set) {
                   return std::make_unique<SetInNode>(variable, set.values);
                 },
                 [&](const fznparser::IntRange& range) {
                   std::vector<Int> values;
                   values.resize(range.upperBound - range.lowerBound + 1);
                   std::iota(values.begin(), values.end(), range.lowerBound);
                   return std::make_unique<SetInNode>(variable, values);
                 }},
      valueSet);
}

void invariantgraph::SetInNode::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  VarId violation = registerViolation(engine, variableMap);
  VarId input = variableMap.at(_input);

  std::vector<DomainEntry> domainEntries;
  domainEntries.reserve(_values.size());
  std::transform(_values.begin(), _values.end(),
                 std::back_inserter(domainEntries),
                 [](const auto& value) { return DomainEntry(value, value); });

  engine.makeConstraint<InDomain>(violation, input, std::move(domainEntries));
}
