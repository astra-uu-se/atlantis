#include "atlantis/invariantgraph/invariantNodes/arrayIntMinimumNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/minSparse.hpp"
#include "atlantis/propagation/views/intMinView.hpp"

namespace atlantis::invariantgraph {

ArrayIntMinimumNode::ArrayIntMinimumNode(VarNodeId a, VarNodeId b,
                                         VarNodeId output)
    : ArrayIntMinimumNode(std::vector<VarNodeId>{a, b}, output) {}

ArrayIntMinimumNode::ArrayIntMinimumNode(std::vector<VarNodeId>&& vars,
                                         VarNodeId output)
    : InvariantNode({output}, std::move(vars)),
      _ub(std::numeric_limits<Int>::max()) {}

void ArrayIntMinimumNode::init(InvariantGraph& graph,
                               const InvariantNodeId& id) {
  InvariantNode::init(graph, id);
  assert(graph.varNodeConst(outputVarNodeIds().front()).isIntVar());
  assert(std::all_of(staticInputVarNodeIds().begin(),
                     staticInputVarNodeIds().end(), [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).isIntVar();
                     }));
}

void ArrayIntMinimumNode::updateState(InvariantGraph& graph) {
  Int lb = _ub;
  for (const auto& input : staticInputVarNodeIds()) {
    lb = std::min(lb, graph.varNodeConst(input).lowerBound());
    _ub = std::min(_ub, graph.varNodeConst(input).upperBound());
  }
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());

  for (const auto& input : staticInputVarNodeIds()) {
    if (graph.varNodeConst(input).lowerBound() >= _ub) {
      varsToRemove.emplace_back(input);
    }
  }
  for (const auto& input : varsToRemove) {
    removeStaticInputVarNode(graph.varNode(input));
  }
  auto& outputVar = graph.varNode(outputVarNodeIds().front());
  outputVar.removeValuesBelow(lb);
  outputVar.removeValuesAbove(_ub);
  if (staticInputVarNodeIds().size() == 0 || outputVar.isFixed()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool ArrayIntMinimumNode::canBeReplaced(const InvariantGraph& graph) const {
  if (state() != InvariantNodeState::ACTIVE ||
      staticInputVarNodeIds().size() > 1) {
    return false;
  }
  if (staticInputVarNodeIds().empty()) {
    return true;
  }
  return _ub >=
         graph.varNodeConst(staticInputVarNodeIds().front()).upperBound();
}

bool ArrayIntMinimumNode::replace(InvariantGraph& graph) {
  if (!canBeReplaced(graph)) {
    return false;
  }
  if (staticInputVarNodeIds().size() == 1) {
    graph.replaceVarNode(outputVarNodeIds().front(),
                         staticInputVarNodeIds().front());
  }
  return true;
}

void ArrayIntMinimumNode::registerOutputVars(InvariantGraph& graph,
                                             propagation::SolverBase& solver) {
  if (staticInputVarNodeIds().size() == 1) {
    graph.varNode(outputVarNodeIds().front())
        .setVarId(solver.makeIntView<propagation::IntMinView>(
            solver, graph.varId(staticInputVarNodeIds().front()), _ub));
  } else if (!staticInputVarNodeIds().empty()) {
    makeSolverVar(solver, graph.varNode(outputVarNodeIds().front()));
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void ArrayIntMinimumNode::registerNode(InvariantGraph& graph,
                                       propagation::SolverBase& solver) {
  if (staticInputVarNodeIds().size() <= 1) {
    return;
  }
  std::vector<propagation::VarId> solverVars;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars),
                 [&](const auto& node) { return graph.varId(node); });

  assert(graph.varId(outputVarNodeIds().front()) != propagation::NULL_ID);
  solver.makeInvariant<propagation::MinSparse>(
      solver, graph.varId(outputVarNodeIds().front()), std::move(solverVars));
}

}  // namespace atlantis::invariantgraph
