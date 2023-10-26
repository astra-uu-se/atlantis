#include "invariantgraph/implicitConstraints/circuitImplicitNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

CircuitImplicitNode::CircuitImplicitNode(std::vector<VarNodeId>&& vars)
    : ImplicitConstraintNode(std::move(vars)) {
  assert(outputVarNodeIds().size() > 1);
}

std::unique_ptr<CircuitImplicitNode> CircuitImplicitNode::fromModelConstraint(
    const fznparser::Constraint& constraint,
    FznInvariantGraph& invariantGraph) {
  assert(hasCorrectSignature(acceptedNameNumArgPairs(), constraint));

  const fznparser::IntVarArray& arg =
      std::get<fznparser::IntVarArray>(constraint.arguments().at(0));

  if (arg.size() < 2) {
    return nullptr;
  }

  for (size_t i = 0; i < arg.size(); ++i) {
    if (std::holds_alternative<Int>(arg.at(i))) {
      return nullptr;
    }
  }

  // For now, this only works when all the vars have the same domain.
  const fznparser::IntSet& domain =
      std::get<std::reference_wrapper<const fznparser::IntVar>>(arg.at(0))
          .get()
          .domain();

  for (size_t i = 1; i < arg.size(); ++i) {
    if (domain !=
        std::get<std::reference_wrapper<const fznparser::IntVar>>(arg.at(i))
            .get()
            .domain()) {
      return nullptr;
    }
  }

  return std::make_unique<CircuitImplicitNode>(
      invariantGraph.createVarNodes(arg));
}

std::shared_ptr<search::neighbourhoods::Neighbourhood>
CircuitImplicitNode::createNeighbourhood(
    propagation::SolverBase&, std::vector<search::SearchVar>&& vars) {
  return std::make_shared<search::neighbourhoods::CircuitNeighbourhood>(
      std::move(vars));
}

}  // namespace atlantis::invariantgraph