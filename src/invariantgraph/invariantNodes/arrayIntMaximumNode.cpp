#include "atlantis/invariantgraph/invariantNodes/arrayIntMaximumNode.hpp"

#include <algorithm>
#include <limits>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/invariants/max.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/intMaxView.hpp"

namespace atlantis::invariantgraph {

ArrayIntMaximumNode::ArrayIntMaximumNode(InvariantGraph& graph, VarNodeId a,
                                         VarNodeId b, VarNodeId output)
    : ArrayIntMaximumNode(graph, std::vector<VarNodeId>{a, b}, output) {}

ArrayIntMaximumNode::ArrayIntMaximumNode(InvariantGraph& graph,
                                         std::vector<VarNodeId>&& vars,
                                         VarNodeId output)
    : InvariantNode(graph, {output}, std::move(vars)),
      _lb{std::numeric_limits<Int>::min()} {}

void ArrayIntMaximumNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(invariantGraphConst()
             .varNodeConst(outputVarNodeIds().front())
             .isIntVar());
  assert(std::ranges::all_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
}

void ArrayIntMaximumNode::updateState() {
  auto& outNode = invariantGraph().varNode(outputVarNodeIds().front());

  Int ub = _lb;
  for (const auto& input : staticInputVarNodeIds()) {
    auto& vNode = invariantGraph().varNode(input);
    vNode.removeValuesAbove(outNode.upperBound());
    _lb = std::max(_lb, vNode.lowerBound());
    ub = std::max(ub, vNode.upperBound());
  }
  outNode.removeValuesBelow(_lb);
  outNode.removeValuesAbove(ub);

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
  if (staticInputVarNodeIds().empty()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool ArrayIntMaximumNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         staticInputVarNodeIds().size() == 1 &&
         _lb <= invariantGraphConst().varNodeConst(staticInputVarNodeIds().front()).lowerBound();
}

bool ArrayIntMaximumNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  invariantGraph().replaceVarNode(outputVarNodeIds().front(),
                                  staticInputVarNodeIds().front());
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
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
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
  solverVars.reserve(staticInputVarNodeIds().size());
  std::ranges::transform(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      std::back_inserter(solverVars),
      [&](const auto& node) { return invariantGraph().varId(node); });

  assert(invariantGraph().varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);
  assert(invariantGraph().varId(outputVarNodeIds().front()).isVar());
  solver().makeInvariant<propagation::Max>(
      solver(), invariantGraph().varId(outputVarNodeIds().front()),
      std::move(solverVars));
}

std::string ArrayIntMaximumNode::dotLangIdentifier() const { return "max"; }

}  // namespace atlantis::invariantgraph
