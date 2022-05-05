#include "invariantgraph/constraints/setInNode.hpp"

#include "../parseHelper.hpp"
#include "constraints/inDomain.hpp"
#include "utils/variant.hpp"

std::unique_ptr<invariantgraph::SetInNode>
invariantgraph::SetInNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(
      (constraint.name == "set_in" && constraint.arguments.size() == 2) ||
      (constraint.name == "set_in_reif" && constraint.arguments.size() == 3));

  auto variable = mappedVariable(constraint.arguments[0], variableMap);
  auto valueSet = integerSet(model, constraint.arguments[1]);
  VariableNode* r = constraint.arguments.size() >= 3
                        ? mappedVariable(constraint.arguments[2], variableMap)
                        : nullptr;

  // Note: if the valueSet is an IntRange, here all the values are collected
  // into a vector. If it turns out memory usage is an issue, this should be
  // mitigated.

  return std::visit<std::unique_ptr<SetInNode>>(
      overloaded{[&](const fznparser::LiteralSet<Int>& set) {
                   return std::make_unique<SetInNode>(variable, set.values, r);
                 },
                 [&](const fznparser::IntRange& range) {
                   std::vector<Int> values;
                   values.resize(range.upperBound - range.lowerBound + 1);
                   std::iota(values.begin(), values.end(), range.lowerBound);
                   return std::make_unique<SetInNode>(variable, values, r);
                 }},
      valueSet);
}

void invariantgraph::SetInNode::createDefinedVariables(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  if (!variableMap.contains(violation())) {
    registerViolation(engine, variableMap);
  }
}

void invariantgraph::SetInNode::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  VarId input = variableMap.at(staticInputs().front());

  std::vector<DomainEntry> domainEntries;
  domainEntries.reserve(_values.size());
  std::transform(_values.begin(), _values.end(),
                 std::back_inserter(domainEntries),
                 [](const auto& value) { return DomainEntry(value, value); });

  assert(variableMap.contains(violation()));

  engine.makeConstraint<InDomain>(variableMap.at(violation()), input,
                                  std::move(domainEntries));
}
