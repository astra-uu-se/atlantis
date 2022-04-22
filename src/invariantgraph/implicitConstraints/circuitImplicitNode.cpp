#include "invariantgraph/implicitConstraints/circuitImplicitNode.hpp"

std::unique_ptr<invariantgraph::CircuitImplicitNode>
invariantgraph::CircuitImplicitNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "circuit");
  assert(constraint.arguments.size() == 1);

  auto variables =
      mappedVariableVector(model, constraint.arguments[0], variableMap);

  for (size_t i = 0; i < variables.size(); ++i) {
    const std::vector<Int>& domain = std::visit<std::vector<Int>&>(
        [&](const auto& dom) { return domain.values(); },
        variables[i]->domain());
    if (domain.size() == 0 || (domain.size() == 1 && domain.front() == i + 1)) {
      return nullptr;
    }
  }

  return std::make_unique<CircuitImplicitNode>(variables);
}

invariantgraph::CircuitImplicitNode::CircuitImplicitNode(
    std::vector<VariableNode*> variables)
    : ImplicitConstraintNode(std::move(variables)) {}

search::neighbourhoods::Neighbourhood*
invariantgraph::CircuitImplicitNode::createNeighbourhood(
    Engine& engine, std::vector<search::SearchVariable> variables) {
  std::vector<std::vector<Int>> domainValues;
  domainValues.reserve(variables.size());
  for (const auto* definedVariable : definedVariables()) {
    domainValues.emplace_back(
        std::visit([&](auto& domain) { return domain.values(); },
                   definedVariable->domain()));
  }

  return new search::neighbourhoods::CircuitNeighbourhood(variables,
                                                          domainValues, engine);
}
