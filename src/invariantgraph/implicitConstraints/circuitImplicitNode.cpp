#include "invariantgraph/implicitConstraintNodes/circuitImplicitNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

CircuitImplicitNode::CircuitImplicitNode(std::vector<VarNodeId>&& vars)
    : ImplicitConstraintNode(std::move(vars)) {
  assert(outputVarNodeIds().size() > 1);
}

std::shared_ptr<search::neighbourhoods::Neighbourhood>
CircuitImplicitNode::createNeighbourhood(
    propagation::SolverBase&, std::vector<search::SearchVar>&& vars) {
  return std::make_shared<search::neighbourhoods::CircuitNeighbourhood>(
      std::move(vars));
}

}  // namespace atlantis::invariantgraph