#include "invariantgraph/implicitConstraints/circuitImplicitNode.hpp"

#include <numeric>

#include "../parseHelper.hpp"
#include "search/neighbourhoods/circuitNeighbourhood.hpp"

std::unique_ptr<invariantgraph::CircuitImplicitNode>
invariantgraph::CircuitImplicitNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));
  auto variables =
      mappedVariableVector(model, constraint.arguments[0], variableMap);

  // For now, this only works when all the variables have the same domain.
  const auto& domain = variables.front()->domain();
  for (const auto& variable : variables) {
    if (variable->domain() != domain) {
      return nullptr;
    }
  }

  return std::make_unique<CircuitImplicitNode>(variables);
}

invariantgraph::CircuitImplicitNode::CircuitImplicitNode(
    std::vector<VariableNode*> variables)
    : ImplicitConstraintNode(std::move(variables)) {
  assert(definedVariables().size() > 1);

  assert(std::all_of(definedVariables().begin(), definedVariables().end(),
                     [&](VariableNode* const variable) {
                       return variable->domain() ==
                              definedVariables().front()->domain();
                     }));
}

search::neighbourhoods::Neighbourhood*
invariantgraph::CircuitImplicitNode::createNeighbourhood(
    Engine&, std::vector<search::SearchVariable> variables) {
  return new search::neighbourhoods::CircuitNeighbourhood(variables);
}
