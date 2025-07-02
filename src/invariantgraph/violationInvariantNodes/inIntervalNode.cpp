#include "atlantis/invariantgraph/violationInvariantNodes/inIntervalNode.hpp"

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/fzn/fzn_all_different_int.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/inIntervalConst.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

InIntervalNode::InIntervalNode(InvariantGraph& graph, VarNodeId input, Int lb,
                               Int ub, VarNodeId r)
    : ViolationInvariantNode(graph, {input}, r), _lb(lb), _ub(ub) {}

InIntervalNode::InIntervalNode(InvariantGraph& graph, VarNodeId input, Int lb,
                               Int ub, bool shouldHold)
    : ViolationInvariantNode(graph, {input}, shouldHold), _lb(lb), _ub(ub) {}

void InIntervalNode::init(InvariantNodeId id) {
  ViolationInvariantNode::init(id);
  assert(
      !isReified() ||
      !invariantGraphConst().varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(std::ranges::all_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
}
void InIntervalNode::updateState() {
  ViolationInvariantNode::updateState();
  if (_ub < _lb) {
    if (isReified()) {
      fixReified(false);
    } else if (shouldHold()) {
      throw InconsistencyException("InIntervalNode::updateState: empty set");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  auto& vNode = invariantGraph().varNode(staticInputVarNodeIds().front());
  if (!isReified()) {
    if (shouldHold()) {
      vNode.domain()->removeAllValuesExcept(_lb, _ub);
    } else {
      vNode.domain()->remove(_lb, _ub);
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (vNode.constDomain()->isDisjoint(_lb, _ub)) {
    fixReified(false);
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (vNode.constDomain()->isContained(_lb, _ub)) {
    fixReified(true);
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
}

void InIntervalNode::registerOutputVars() {
  if (violationVarId() == propagation::NULL_ID) {
    if (shouldHold()) {
      setViolationVarId(solver().makeIntView<propagation::InIntervalConst>(
          solver(), invariantGraph().varId(staticInputVarNodeIds().front()),
          _lb, _ub));
    } else {
      assert(!isReified());
      _intermediate = solver().makeIntView<propagation::InIntervalConst>(
          solver(), invariantGraph().varId(staticInputVarNodeIds().front()),
          _lb, _ub);
      setViolationVarId(solver().makeIntView<propagation::NotEqualConst>(
          solver(), _intermediate, 0));
    }
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).varId() !=
               propagation::NULL_ID;
      }));
}

void InIntervalNode::registerNode() {}

std::string InIntervalNode::dotLangIdentifier() const { return "in_interval"; }

}  // namespace atlantis::invariantgraph
