#include "invariantgraph/implicitConstraints/allDifferentImplicitNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

std::unique_ptr<AllDifferentImplicitNode>
AllDifferentImplicitNode::fromModelConstraint(
    const fznparser::Constraint& constraint, InvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  auto arg = std::get<fznparser::IntVarArray>(constraint.arguments().at(0));

  if (arg.size() < 2) {
    // Apparently it can happen that variables is an array of length 1. In that
    // case, there is no benefit by the variable being defined by this implicit
    // node, since any value from its domain would satisfy this constraint.
    return nullptr;
  }

  return std::make_unique<AllDifferentImplicitNode>(
      invariantGraph.createVarNodes(arg));
}

AllDifferentImplicitNode::AllDifferentImplicitNode(
    std::vector<VarNodeId>&& variables)
    : ImplicitConstraintNode(std::move(variables)) {
  assert(outputVarNodeIds().size() > 1);
}

bool AllDifferentImplicitNode::prune(InvariantGraph& invariantGraph) {
  std::vector<VarNodeId> fixedInputNodes =
      pruneAllDifferentFixed(invariantGraph, outputVarNodeIds());

  for (const auto& singleton : fixedInputNodes) {
    removeOutputVarNode(invariantGraph.varNode(singleton));
  }

  return !fixedInputNodes.empty();
}

std::shared_ptr<search::neighbourhoods::Neighbourhood>
AllDifferentImplicitNode::createNeighbourhood(
    propagation::Engine& engine, std::vector<search::SearchVariable>&& variables) {
  if (variables.size() <= 1) {
    return nullptr;
  }
  bool hasSameDomain = true;
  assert(!variables.empty());

  auto& domain = variables.front().domain();
  for (auto& variable : variables) {
    if (variable.domain() != domain) {
      hasSameDomain = false;
      break;
    }
  }
  if (hasSameDomain) {
    return std::make_shared<
        search::neighbourhoods::AllDifferentUniformNeighbourhood>(
        std::move(variables), domain.values(), engine);
  } else {
    Int domainLb = std::numeric_limits<Int>::max();
    Int domainUb = std::numeric_limits<Int>::min();
    for (auto& variable : variables) {
      domainLb = std::min<Int>(domainLb, variable.domain().lowerBound());
      domainUb = std::max<Int>(domainUb, variable.domain().upperBound());
    }
    return std::make_shared<
        search::neighbourhoods::AllDifferentNonUniformNeighbourhood>(
        std::move(variables), domainLb, domainUb, engine);
  }
}

}  // namespace invariantgraph