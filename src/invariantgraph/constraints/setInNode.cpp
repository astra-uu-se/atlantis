#include "invariantgraph/constraints/setInNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::SetInNode>
invariantgraph::SetInNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto variable = mappedVariable(constraint.arguments[0], variableMap);
  auto valueSet = integerSet(model, constraint.arguments[1]);

  // Note: if the valueSet is an IntRange, here all the values are collected
  // into a vector. If it turns out memory usage is an issue, this should be
  // mitigated.

  std::vector<Int> values = std::visit<std::vector<Int>>(
      overloaded{
          [&](const fznparser::LiteralSet<Int>& set) { return set.values; },
          [&](const fznparser::IntRange& range) {
            std::vector<Int> vals;
            vals.resize(range.upperBound - range.lowerBound + 1);
            std::iota(vals.begin(), vals.end(), range.lowerBound);
            return vals;
          }},
      valueSet);

  if (constraint.arguments.size() >= 3) {
    if (std::holds_alternative<bool>(constraint.arguments[2])) {
      auto shouldHold = std::get<bool>(constraint.arguments[2]);
      return std::make_unique<SetInNode>(variable, values, shouldHold);
    } else {
      auto r = mappedVariable(constraint.arguments[2], variableMap);
      return std::make_unique<SetInNode>(variable, values, r);
    }
  }
  return std::make_unique<SetInNode>(variable, values, true);
}

void invariantgraph::SetInNode::createDefinedVariables(Engine& engine) {
  if (violationVarId() == NULL_ID) {
    const VarId input = staticInputs().front()->varId();
    std::vector<DomainEntry> domainEntries;
    domainEntries.reserve(_values.size());
    std::transform(_values.begin(), _values.end(),
                   std::back_inserter(domainEntries),
                   [](const auto& value) { return DomainEntry(value, value); });

    if (!shouldHold()) {
      assert(!isReified());
      _intermediate =
          engine.makeIntView<InDomain>(input, std::move(domainEntries));
      setViolationVarId(engine.makeIntView<NotEqualConst>(_intermediate, 0));
    } else {
      setViolationVarId(
          engine.makeIntView<InDomain>(input, std::move(domainEntries)));
    }
  }
}

void invariantgraph::SetInNode::registerWithEngine(Engine&) {}
