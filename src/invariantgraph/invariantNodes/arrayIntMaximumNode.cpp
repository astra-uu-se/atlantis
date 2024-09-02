#include "atlantis/invariantgraph/invariantNodes/arrayIntMaximumNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/maxSparse.hpp"
#include "atlantis/propagation/views/intMaxView.hpp"

namespace atlantis::invariantgraph {

ArrayIntMaximumNode::ArrayIntMaximumNode(InvariantGraph& graph, VarNodeId a,
                                         VarNodeId b, VarNodeId output)
    : ArrayIntMaximumNode(graph, solver(), std::vector<VarNodeId>{a, b},
                          output) {}

ArrayIntMaximumNode::ArrayIntMaximumNode(InvariantGraph& graph,
                                         std::vector<VarNodeId>&& vars,
                                         VarNodeId output)
    : InvariantNode(graph, solver(), {output}, std::move(vars)),
      _lb{std::numeric_limits<Int>::min()} {}

void ArrayIntMaximumNode::init(const InvariantNodeId& id) {
  InvariantNode::init(id);
  assert(graph.varNodeConst(outputVarNodeIds().front()).isIntVar());
  assert(std::all_of(staticInputVarNodeIds().begin(),
                     staticInputVarNodeIds().end(), [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).isIntVar();
                     }));
}

void ArrayIntMaximumNode::updateState() {
  Int ub = _lb;
  for (const auto& input : staticInputVarNodeIds()) {
    _lb = std::max(_lb, graph.varNodeConst(input).lowerBound());
    ub = std::max(ub, graph.varNodeConst(input).upperBound());
  }
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());

  for (const auto& input : staticInputVarNodeIds()) {
    if (graph.varNodeConst(input).upperBound() <= _lb) {
      varsToRemove.emplace_back(input);
    }
  }
  for (const auto& input : varsToRemove) {
    removeStaticInputVarNode(graph.varNode(input));
  }
  auto& outputVar = graph.varNode(outputVarNodeIds().front());
  // outputVar.removeValuesBelow(_lb);
  // outputVar.removeValuesAbove(ub);
  if (staticInputVarNodeIds().size() == 0 || outputVar.isFixed()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool ArrayIntMaximumNode::canBeReplaced() const {
  if (state() != InvariantNodeState::ACTIVE ||
      staticInputVarNodeIds().size() > 1) {
    return false;
  }
  if (staticInputVarNodeIds().empty()) {
    return true;
  }
  return _lb <=
         graph.varNodeConst(staticInputVarNodeIds().front()).lowerBound();
}

bool ArrayIntMaximumNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (staticInputVarNodeIds().size() == 1) {
    graph.replaceVarNode(outputVarNodeIds().front(),
                         staticInputVarNodeIds().front());
  }
  return true;
}

void ArrayIntMaximumNode::registerOutputVars() {
  if (staticInputVarNodeIds().size() == 1) {
    graph.varNode(outputVarNodeIds().front())
        .setVarId(solver().makeIntView<propagation::IntMaxView>(
            solver(), graph.varId(staticInputVarNodeIds().front()), _lb));
  } else if (!staticInputVarNodeIds().empty()) {
    makeSolverVar(solver(), graph.varNode(outputVarNodeIds().front()));
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void ArrayIntMaximumNode::registerNode() {
  if (staticInputVarNodeIds().size() <= 1) {
    return;
  }
  std::vector<propagation::VarId> solverVars;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars),
                 [&](const auto& node) { return graph.varId(node); });

  assert(graph.varId(outputVarNodeIds().front()) != propagation::NULL_ID);
  solver().makeInvariant<propagation::MaxSparse>(
      solver(), graph.varId(outputVarNodeIds().front()), std::move(solverVars));
}

}  // namespace atlantis::invariantgraph
