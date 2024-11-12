#include "atlantis/invariantgraph/invariantNodes/arrayIntMinimumNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/min.hpp"
#include "atlantis/propagation/views/intMinView.hpp"

namespace atlantis::invariantgraph {

ArrayIntMinimumNode::ArrayIntMinimumNode(IInvariantGraph& graph, VarNodeId a,
                                         VarNodeId b, VarNodeId output)
    : ArrayIntMinimumNode(graph, std::vector<VarNodeId>{a, b}, output) {}

ArrayIntMinimumNode::ArrayIntMinimumNode(IInvariantGraph& graph,
                                         std::vector<VarNodeId>&& vars,
                                         VarNodeId output)
    : InvariantNode(graph, {output}, std::move(vars)),
      _ub(std::numeric_limits<Int>::max()) {}

void ArrayIntMinimumNode::init(InvariantNodeId id) {
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

void ArrayIntMinimumNode::updateState() {
  Int lb = _ub;
  for (const auto& input : staticInputVarNodeIds()) {
    lb = std::min(lb, invariantGraphConst().varNodeConst(input).lowerBound());
    _ub = std::min(_ub, invariantGraphConst().varNodeConst(input).upperBound());
  }
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());

  for (const auto& input : staticInputVarNodeIds()) {
    if (invariantGraphConst().varNodeConst(input).lowerBound() >= _ub) {
      varsToRemove.emplace_back(input);
    }
  }
  for (const auto& input : varsToRemove) {
    removeStaticInputVarNode(input);
  }
  auto& outputVar = invariantGraph().varNode(outputVarNodeIds().front());
  // outputVar.removeValuesBelow(lb);
  // outputVar.removeValuesAbove(_ub);
  if (staticInputVarNodeIds().size() == 0 || outputVar.isFixed()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool ArrayIntMinimumNode::canBeReplaced() const {
  if (state() != InvariantNodeState::ACTIVE ||
      staticInputVarNodeIds().size() > 1) {
    return false;
  }
  if (staticInputVarNodeIds().empty()) {
    return true;
  }
  return _ub >= invariantGraphConst()
                    .varNodeConst(staticInputVarNodeIds().front())
                    .upperBound();
}

bool ArrayIntMinimumNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (staticInputVarNodeIds().size() == 1) {
    invariantGraph().replaceVarNode(outputVarNodeIds().front(),
                                    staticInputVarNodeIds().front());
  }
  return true;
}

void ArrayIntMinimumNode::registerOutputVars() {
  if (staticInputVarNodeIds().size() == 1) {
    invariantGraph()
        .varNode(outputVarNodeIds().front())
        .setVarId(solver().makeIntView<propagation::IntMinView>(
            solver(), invariantGraph().varId(staticInputVarNodeIds().front()),
            _ub));
  } else if (!staticInputVarNodeIds().empty()) {
    makeSolverVar(outputVarNodeIds().front());
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId vId) {
                       return invariantGraphConst().varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void ArrayIntMinimumNode::registerNode() {
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
  solver().makeInvariant<propagation::Min>(
      solver(), invariantGraph().varId(outputVarNodeIds().front()),
      std::move(solverVars));
}

}  // namespace atlantis::invariantgraph
