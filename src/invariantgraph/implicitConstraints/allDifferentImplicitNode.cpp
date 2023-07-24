#include "invariantgraph/implicitConstraints/allDifferentImplicitNode.hpp"

#include "../parseHelper.hpp"

namespace invariantgraph {

std::unique_ptr<AllDifferentImplicitNode>
AllDifferentImplicitNode::fromModelConstraint(
    const fznparser::Model&, const fznparser::Constraint& constraint,
    InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto arg = std::get<fznparser::IntVarArray>(constraint.arguments().at(0));

  if (arg.size() < 2) {
    // Apparently it can happen that variables is an array of length 1. In that
    // case, there is no benefit by the variable being defined by this implicit
    // node, since any value from its domain would satisfy this constraint.
    return nullptr;
  }

  return std::make_unique<AllDifferentImplicitNode>(
      invariantGraph.addVariableArray(arg));
}

AllDifferentImplicitNode::AllDifferentImplicitNode(
    std::vector<VariableNode*> variables)
    : ImplicitConstraintNode(std::move(variables)) {
  assert(definedVariables().size() > 1);
}

bool AllDifferentImplicitNode::prune() {
  std::vector<VariableNode*> singletonDefinedVariables =
      pruneAllDifferent(definedVariables());
  for (auto* const singleton : singletonDefinedVariables) {
    removeDefinedVariable(singleton);
  }
  return !singletonDefinedVariables.empty();
}

search::neighbourhoods::Neighbourhood*
AllDifferentImplicitNode::createNeighbourhood(
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
    return new search::neighbourhoods::AllDifferentUniformNeighbourhood(
        variables, std::move(definedVariables().front()->domain().values()),
        engine);
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

}  // namespace invariantgraph