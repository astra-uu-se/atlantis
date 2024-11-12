#include "atlantis/invariantgraph/invariantNodes/arrayIntMaximumNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/max.hpp"
#include "atlantis/propagation/views/intMaxView.hpp"

namespace atlantis::invariantgraph {

ArrayIntMaximumNode::ArrayIntMaximumNode(IInvariantGraph& graph, VarNodeId a,
                                         VarNodeId b, VarNodeId output)
    : ArrayIntMaximumNode(graph, std::vector<VarNodeId>{a, b}, output) {}

ArrayIntMaximumNode::ArrayIntMaximumNode(IInvariantGraph& graph,
                                         std::vector<VarNodeId>&& vars,
                                         VarNodeId output)
    : InvariantNode(graph, {output}, std::move(vars)),
      _lb{std::numeric_limits<Int>::min()} {}

void ArrayIntMaximumNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(invariantGraphConst()
             .varNodeConst(outputVarNodeIds().front())
             .isIntVar());
  assert(
      std::all_of(staticInputVarNodeIds().begin(),
                  staticInputVarNodeIds().end(), [&](const VarNodeId vId) {
                    return invariantGraphConst().varNodeConst(vId).isIntVar();
                  }));
}

void ArrayIntMaximumNode::updateState() {
  Int ub = _lb;
  for (const auto& input : staticInputVarNodeIds()) {
    _lb = std::max(_lb, invariantGraphConst().varNodeConst(input).lowerBound());
    ub = std::max(ub, invariantGraphConst().varNodeConst(input).upperBound());
  }
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());

  for (const auto& input : staticInputVarNodeIds()) {
    if (invariantGraphConst().varNodeConst(input).upperBound() <= _lb) {
      varsToRemove.emplace_back(input);
    }
  }
  for (const auto& input : varsToRemove) {
    removeStaticInputVarNode(input);
  }
  auto& outputVar = invariantGraph().varNode(outputVarNodeIds().front());
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
  return _lb <= invariantGraphConst()
                    .varNodeConst(staticInputVarNodeIds().front())
                    .lowerBound();
}

bool ArrayIntMaximumNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (staticInputVarNodeIds().size() == 1) {
    invariantGraph().replaceVarNode(outputVarNodeIds().front(),
                                    staticInputVarNodeIds().front());
  }
  return true;
}

void ArrayIntMaximumNode::registerOutputVars() {
  if (staticInputVarNodeIds().size() == 1) {
    invariantGraph()
        .varNode(outputVarNodeIds().front())
        .setVarId(solver().makeIntView<propagation::IntMaxView>(
            solver(), invariantGraph().varId(staticInputVarNodeIds().front()),
            _lb));
  } else if (!staticInputVarNodeIds().empty()) {
    makeSolverVar(outputVarNodeIds().front());
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId vId) {
                       return invariantGraphConst().varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void ArrayIntMaximumNode::registerNode() {
  if (staticInputVarNodeIds().size() <= 1) {
    return;
  }
  std::vector<propagation::VarViewId> solverVars;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars), [&](const auto& node) {
                   return invariantGraph().varId(node);
                 });

  assert(invariantGraph().varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);
  assert(invariantGraph().varId(outputVarNodeIds().front()).isVar());
  solver().makeInvariant<propagation::Max>(
      solver(), invariantGraph().varId(outputVarNodeIds().front()),
      std::move(solverVars));
}

}  // namespace atlantis::invariantgraph
