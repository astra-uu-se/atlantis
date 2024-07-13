#include "atlantis/invariantgraph/invariantNodes/intTimesNode.hpp"

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/times.hpp"
#include "atlantis/propagation/views/scalarView.hpp"

namespace atlantis::invariantgraph {

IntTimesNode::IntTimesNode(VarNodeId a, VarNodeId b, VarNodeId output)
    : InvariantNode({output}, {a, b}) {}

void IntTimesNode::init(InvariantGraph& graph, const InvariantNodeId& id) {
  InvariantNode::init(graph, id);
  assert(graph.varNodeConst(outputVarNodeIds().front()).isIntVar());
  assert(std::all_of(staticInputVarNodeIds().begin(),
                     staticInputVarNodeIds().end(), [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).isIntVar();
                     }));
}

void IntTimesNode::updateState(InvariantGraph& graph) {
  std::vector<VarNodeId> varNodeIdsToRemove;
  varNodeIdsToRemove.reserve(staticInputVarNodeIds().size());

  for (const auto& varNodeId : staticInputVarNodeIds()) {
    if (graph.varNodeConst(varNodeId).isFixed()) {
      varNodeIdsToRemove.emplace_back(varNodeId);
      _scalar *= graph.varNodeConst(varNodeId).lowerBound();
    }
  }

  if (_scalar == 0) {
    graph.varNode(outputVarNodeIds().front()).fixToValue(Int{0});
    setState(InvariantNodeState::SUBSUMED);
    return;
  }

  for (const auto& varNodeId : varNodeIdsToRemove) {
    removeStaticInputVarNode(graph.varNode(varNodeId));
  }

  Int lb = _scalar;
  Int ub = _scalar;
  for (const auto& inputId : staticInputVarNodeIds()) {
    const auto& inputNode = graph.varNodeConst(inputId);
    lb = std::min(lb * inputNode.lowerBound(), lb * inputNode.upperBound());
    ub = std::max(ub * inputNode.lowerBound(), ub * inputNode.upperBound());
  }

  auto& outputNode = graph.varNode(outputVarNodeIds().front());

  outputNode.removeValuesBelow(lb);
  outputNode.removeValuesAbove(ub);

  if (staticInputVarNodeIds().empty()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool IntTimesNode::canBeReplaced(const InvariantGraph&) const {
  return state() == InvariantNodeState::ACTIVE &&
         staticInputVarNodeIds().size() <= 1 && _scalar == 1;
}

bool IntTimesNode::replace(InvariantGraph& graph) {
  if (!canBeReplaced(graph)) {
    return false;
  }

  if (staticInputVarNodeIds().size() == 1) {
    graph.replaceVarNode(outputVarNodeIds().front(),
                         staticInputVarNodeIds().front());
  }

  return true;
}

void IntTimesNode::registerOutputVars(InvariantGraph& graph,
                                      propagation::SolverBase& solver) {
  if (!staticInputVarNodeIds().empty()) {
    if (_scalar != 1) {
      if (staticInputVarNodeIds().size() == 1) {
        graph.varNode(outputVarNodeIds().front())
            .setVarId(solver.makeIntView<propagation::ScalarView>(
                solver, graph.varId(staticInputVarNodeIds().front()), _scalar));
      } else {
        _intermediate = solver.makeIntVar(0, 0, 0);
        graph.varNode(outputVarNodeIds().front())
            .setVarId(solver.makeIntView<propagation::ScalarView>(
                solver, _intermediate, _scalar));
      }
    } else {
      makeSolverVar(solver, graph.varNode(outputVarNodeIds().front()));
    }
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void IntTimesNode::registerNode(InvariantGraph& graph,
                                propagation::SolverBase& solver) {
  if (staticInputVarNodeIds().size() <= 1) {
    return;
  }
  assert(graph.varId(outputVarNodeIds().front()) != propagation::NULL_ID);

  solver.makeInvariant<propagation::Times>(
      solver, graph.varId(outputVarNodeIds().front()),
      graph.varId(staticInputVarNodeIds().front()),
      graph.varId(staticInputVarNodeIds().back()));
}

}  // namespace atlantis::invariantgraph
