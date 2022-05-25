#include "invariantgraph/implicitConstraints/allDifferentImplicitNode.hpp"

#include <numeric>

#include "../parseHelper.hpp"
#include "search/neighbourhoods/allDifferentNeighbourhood.hpp"

std::unique_ptr<invariantgraph::AllDifferentImplicitNode>
invariantgraph::AllDifferentImplicitNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "fzn_all_different_int");
  assert(constraint.arguments.size() == 1);

  auto variables =
      mappedVariableVector(model, constraint.arguments[0], variableMap);

  if (variables.size() < 2) {
    // Apparently it can happen that variables is an array of length 1. In that
    // case, there is no benefit by the variable being defined by this implicit
    // node, since any value from its domain would satisfy this constraint.
    return nullptr;
  }

  // For now, this only works when all the variables have the same domain.
  const auto& domain = variables.front()->domain();
  for (const auto& variable : variables) {
    if (variable->domain() != domain) {
      return nullptr;
    }
  }

  return std::make_unique<AllDifferentImplicitNode>(variables);
}

invariantgraph::AllDifferentImplicitNode::AllDifferentImplicitNode(
    std::vector<VariableNode*> variables)
    : ImplicitConstraintNode(std::move(variables)) {
  assert(definedVariables().size() > 1);

  const auto& domain = definedVariables().front()->domain();
  for (const auto& variable : definedVariables()) {
    assert(variable->domain() == domain);
  }
}

search::neighbourhoods::Neighbourhood*
invariantgraph::AllDifferentImplicitNode::createNeighbourhood(
    Engine& engine, std::vector<search::SearchVariable> variables) {
  std::vector<Int> domainValues(
      std::move(definedVariables().front()->domain().values()));

  return new search::neighbourhoods::AllDifferentNeighbourhood(
      variables, std::move(domainValues), engine);
}
