#include "atlantis/invariantgraph/violationInvariantNodes/setInNode.hpp"

#include <boost/xpressive/detail/core/access.hpp>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/fzn/fzn_all_different_int.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/inDomain.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

SetInNode::SetInNode(InvariantGraph& graph, VarNodeId input,
                     std::vector<Int>&& values, VarNodeId r)
    : ViolationInvariantNode(graph, {input}, r), _values(std::move(values)) {}

SetInNode::SetInNode(InvariantGraph& graph, VarNodeId input,
                     std::vector<Int>&& values, bool shouldHold)
    : ViolationInvariantNode(graph, {input}, shouldHold),
      _values(std::move(values)) {}

void SetInNode::init(InvariantNodeId id) {
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

void SetInNode::updateState() {
  ViolationInvariantNode::updateState();
  if ((*_values).empty()) {
    if (isReified()) {
      fixReified(false);
    } else if (shouldHold()) {
      throw InconsistencyException("SetInNode::updateState: empty set");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  auto& vNode = invariantGraph().varNode(staticInputVarNodeIds().front());
  if (!isReified()) {
    if (shouldHold()) {
      vNode.removeAllValuesExcept(_values);
    } else {
      vNode.removeValues(_values);
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (vNode.constDomain()->isDisjoint(_values)) {
    fixReified(false);
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (vNode.constDomain()->isContained(_values)) {
    fixReified(true);
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
}

void SetInNode::registerOutputVars() {
  if (violationVarId() == propagation::NULL_ID) {
    const propagation::VarViewId input =
        invariantGraph().varId(staticInputVarNodeIds().front());
    std::vector<DomainEntry> domainEntries;
    domainEntries.reserve((*_values).size());
    std::ranges::transform(
        *_values, std::back_inserter(domainEntries),
        [](const auto& value) { return DomainEntry(value, value); });

    if (!shouldHold()) {
      assert(!isReified());
      _intermediate = solver().makeIntView<propagation::InDomain>(
          solver(), input, std::move(domainEntries));
      setViolationVarId(solver().makeIntView<propagation::NotEqualConst>(
          solver(), _intermediate, 0));
    } else {
      setViolationVarId(solver().makeIntView<propagation::InDomain>(
          solver(), input, std::move(domainEntries)));
    }
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).varId() !=
               propagation::NULL_ID;
      }));
}

void SetInNode::registerNode() {}

std::string SetInNode::dotLangIdentifier() const { return "set_in"; }

}  // namespace atlantis::invariantgraph
