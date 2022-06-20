#include "invariantgraph/implicitConstraints/allDifferentImplicitNode.hpp"

#include "../parseHelper.hpp"

std::unique_ptr<invariantgraph::AllDifferentImplicitNode>
invariantgraph::AllDifferentImplicitNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto variables = pruneAllDifferent(
      mappedVariableVector(model, constraint.arguments[0], variableMap));

  if (variables.size() < 2) {
    // Apparently it can happen that variables is an array of length 1. In that
    // case, there is no benefit by the variable being defined by this implicit
    // node, since any value from its domain would satisfy this constraint.
    return nullptr;
  }

  return std::make_unique<AllDifferentImplicitNode>(variables);
}

invariantgraph::AllDifferentImplicitNode::AllDifferentImplicitNode(
    std::vector<VariableNode*> variables)
    : ImplicitConstraintNode(std::move(variables)) {
#ifndef NDEBUG
  assert(definedVariables().size() > 1);
}

bool invariantgraph::AllDifferentImplicitNode::prune() {
  std::vector<VariableNode*> singletonStaticInputs =
      pruneAllDifferent(staticInputs());
  for (auto* const singleton : singletonStaticInputs) {
    removeStaticInput(singleton);
  }
  return !singletonStaticInputs.empty();
}

search::neighbourhoods::Neighbourhood*
invariantgraph::AllDifferentImplicitNode::createNeighbourhood(
    Engine& engine, std::vector<search::SearchVariable> variables) {
  bool hasSameDomain = true;
  const auto& domain = variables.front().domain();
  for (auto& variable : variables) {
    if (variable.domain() != domain) {
      hasSameDomain = false;
      break;
    }
  }
  if (hasSameDomain) {
    std::vector<Int> domainValues(
        std::move(definedVariables().front()->domain().values()));

    return new search::neighbourhoods::AllDifferentUniformNeighbourhood(
        variables, std::move(domainValues), engine);
  } else {
    Int domainLb = std::numeric_limits<Int>::max();
    Int domainUb = std::numeric_limits<Int>::min();
    for (auto& variable : variables) {
      domainLb = std::min<Int>(domainLb, variable.domain().lowerBound());
      domainUb = std::max<Int>(domainUb, variable.domain().upperBound());
    }
    return new search::neighbourhoods::AllDifferentNonUniformNeighbourhood(
        variables, domainLb, domainUb, engine);
  }
}
