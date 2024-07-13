#include "atlantis/invariantgraph/invariantNodes/varIntCountNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantNodes/intCountNode.hpp"
#include "atlantis/propagation/invariants/count.hpp"
#include "atlantis/propagation/views/intOffsetView.hpp"
#include "atlantis/propagation/views/scalarView.hpp"

namespace atlantis::invariantgraph {

VarIntCountNode::VarIntCountNode(std::vector<VarNodeId>&& vars,
                                 VarNodeId needle, VarNodeId count)
    : InvariantNode({count}, append(std::move(vars), needle)) {}

void VarIntCountNode::init(InvariantGraph& graph, const InvariantNodeId& id) {
  InvariantNode::init(graph, id);
  assert(graph.varNodeConst(outputVarNodeIds().front()).isIntVar());
  assert(std::all_of(staticInputVarNodeIds().begin(),
                     staticInputVarNodeIds().end(), [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).isIntVar();
                     }));
}

std::vector<VarNodeId> VarIntCountNode::haystack() const {
  std::vector<VarNodeId> inputVarNodeIds;
  inputVarNodeIds.reserve(staticInputVarNodeIds().size() - 1);
  std::copy(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end() - 1,
            std::back_inserter(inputVarNodeIds));
  return inputVarNodeIds;
}

VarNodeId VarIntCountNode::needle() const {
  return staticInputVarNodeIds().back();
}

bool VarIntCountNode::canBeReplaced(const InvariantGraph& graph) const {
  return state() == InvariantNodeState::ACTIVE &&
         graph.varNodeConst(needle()).isFixed();
}

bool VarIntCountNode::replace(InvariantGraph& graph) {
  if (!canBeReplaced(graph)) {
    return false;
  }

  graph.addInvariantNode(std::make_unique<IntCountNode>(
      haystack(), graph.varNodeConst(needle()).lowerBound(),
      outputVarNodeIds().front()));

  return true;
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
