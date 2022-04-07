#include "invariantgraph/constraints/setInNode.hpp"

#include "../parseHelper.hpp"
#include "constraints/inDomain.hpp"

std::unique_ptr<invariantgraph::SetInNode>
invariantgraph::SetInNode::fromModelConstraint(
    const std::shared_ptr<fznparser::Constraint>& constraint,
    const std::function<VariableNode*(std::shared_ptr<fznparser::Variable>)>&
        variableMap) {
  assert(constraint->name() == "set_in");
  assert(constraint->arguments().size() == 2);

  MAPPED_SEARCH_VARIABLE_ARG(variable, constraint->arguments()[0], variableMap);
  VALUE_VECTOR_ARG(values, constraint->arguments()[1]);

  return std::make_unique<SetInNode>(variable, values);
}

void invariantgraph::SetInNode::registerWithEngine(
    Engine& engine, std::map<VariableNode*, VarId>& variableMap) {
  VarId violation = registerViolation(engine, variableMap);
  VarId input = variableMap.at(_input);

  std::vector<DomainEntry> domainEntries;
  domainEntries.reserve(_values.size());
  std::transform(_values.begin(), _values.end(),
                 std::back_inserter(domainEntries),
                 [](const auto& value) { return DomainEntry(value, value); });

  engine.makeConstraint<InDomain>(violation, input, std::move(domainEntries));
}
