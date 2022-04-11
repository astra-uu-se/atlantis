#include "invariantgraph/constraints/setInNode.hpp"

#include "../parseHelper.hpp"
#include "constraints/inDomain.hpp"

std::unique_ptr<invariantgraph::SetInNode>
invariantgraph::SetInNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "set_in");
  assert(constraint.arguments.size() == 2);

  auto variable = mappedVariable(constraint.arguments[0], variableMap);
  auto values = integerVector(model, constraint.arguments[1]);

  return std::make_unique<SetInNode>(variable, values);
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
