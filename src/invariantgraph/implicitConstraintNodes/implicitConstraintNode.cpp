#include "atlantis/invariantgraph/implicitConstraintNode.hpp"

#include <cassert>
#include <vector>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/search/neighbourhoods/neighbourhood.hpp"

namespace atlantis::invariantgraph {

ImplicitConstraintNode::ImplicitConstraintNode(
    std::vector<VarNodeId>&& outputVarNodeIds)
    : InvariantNode(std::move(outputVarNodeIds)) {}

void ImplicitConstraintNode::registerOutputVars(
    InvariantGraph& graph, propagation::SolverBase& solver) {
  for (const auto& varNodeId : outputVarNodeIds()) {
    auto& varNode = graph.varNode(varNodeId);
    if (varNode.varId() == propagation::NULL_ID) {
      const auto& [lb, ub] = varNode.bounds();
      varNode.setVarId(solver.makeIntVar(lb, lb, ub));
    }
  }
}

void ImplicitConstraintNode::init(InvariantGraph& graph,
                                  const InvariantNodeId& id) {
  InvariantNode::init(graph, id);
}

std::shared_ptr<search::neighbourhoods::Neighbourhood>
ImplicitConstraintNode::neighbourhood() noexcept {
  return _neighbourhood;
}

void ImplicitConstraintNode::registerNode(InvariantGraph& graph,
                                          propagation::SolverBase& solver) {
  if (_neighbourhood != nullptr) {
    return;
  }
  _neighbourhood = createNeighbourhood(graph, solver);
  assert(_neighbourhood);
}

std::ostream& ImplicitConstraintNode::dotLangEntry(std::ostream& o) const {
  return o << id() << "[shape=house,label=\"" << dotLangIdentifier() << "\"];"
           << std::endl;
}

}  // namespace atlantis::invariantgraph
