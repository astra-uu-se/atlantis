#include "atlantis/invariantgraph/invariantNodes/varIntCountNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/count.hpp"
#include "atlantis/propagation/views/intOffsetView.hpp"
#include "atlantis/propagation/views/scalarView.hpp"

namespace atlantis::invariantgraph {

VarIntCountNode::VarIntCountNode(std::vector<VarNodeId>&& vars,
                                 VarNodeId needle, VarNodeId count)
    : InvariantNode({count}, append(std::move(vars), needle)) {}

std::vector<VarNodeId> VarIntCountNode::haystack() const {
  std::vector<VarNodeId> inputs;
  inputs.reserve(staticInputVarNodeIds().size() - 1);
  std::copy(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end() - 1,
            std::back_inserter(inputs));
  return inputs;
}

VarNodeId VarIntCountNode::needle() const {
  return staticInputVarNodeIds().back();
}

void VarIntCountNode::propagate(InvariantGraph& graph) {
  size_t haystackSize = staticInputVarNodeIds().size() - 1;
  std::vector<VarNodeId> removedVars;
  removedVars.reserve(haystackSize);

  if (graph.isFixed(outputVarNodeIds().front())) {
    setState(InvariantNodeState::REPLACABLE);
    return;
  }
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size() - 1);
  for (size_t i = 0; i < haystackSize; ++i) {
    if (!graph.varNode(staticInputVarNodeIds().at(i))
             .domainConst()
             .intersects(graph.varNode(needle()).domainConst())) {
      varsToRemove.emplace_back(staticInputVarNodeIds().at(i));
    }
  }
  for (const VarNodeId& input : varsToRemove) {
    removeStaticInputVarNode(graph.varNode(input));
  }
  haystackSize = staticInputVarNodeIds().size() - 1;
  graph.removeValuesBelow(outputVarNodeIds().front(), haystackSize);
}

void VarIntCountNode::registerOutputVars(InvariantGraph& invariantGraph,
                                         propagation::SolverBase& solver) {
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()));
}

void VarIntCountNode::registerNode(InvariantGraph& invariantGraph,
                                   propagation::SolverBase& solver) {
  assert(invariantGraph.varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);

  std::vector<VarNodeId> h = haystack();
  std::vector<propagation::VarId> solverVars;
  solverVars.reserve(h.size());

  std::transform(
      h.begin(), h.end(), std::back_inserter(solverVars),
      [&](const VarNodeId node) { return invariantGraph.varId(node); });

  solver.makeInvariant<propagation::Count>(
      solver, invariantGraph.varId(outputVarNodeIds().front()),
      invariantGraph.varId(needle()), std::move(solverVars));
}

}  // namespace atlantis::invariantgraph
