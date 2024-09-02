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

void VarIntCountNode::init(const InvariantNodeId& id) {
  InvariantNode::init(id);
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

bool VarIntCountNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         graph.varNodeConst(needle()).isFixed();
}

bool VarIntCountNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }

  graph.addInvariantNode(std::make_shared<IntCountNode>(
      graph, haystack(), graph.varNodeConst(needle()).lowerBound(),
      outputVarNodeIds().front()));

  return true;
}

void VarIntCountNode::registerOutputVars() {
  makeSolverVar(solver, graph.varNode(outputVarNodeIds().front()));
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void VarIntCountNode::registerNode() {
  assert(graph.varId(outputVarNodeIds().front()) != propagation::NULL_ID);

  std::vector<VarNodeId> h = haystack();
  std::vector<propagation::VarId> solverVars;
  solverVars.reserve(h.size());

  std::transform(h.begin(), h.end(), std::back_inserter(solverVars),
                 [&](const VarNodeId node) { return graph.varId(node); });

  solver.makeInvariant<propagation::Count>(
      solver, graph.varId(outputVarNodeIds().front()), graph.varId(needle()),
      std::move(solverVars));
}

}  // namespace atlantis::invariantgraph
