#include "atlantis/invariantgraph/violationInvariantNodes/globalCardinalityLowUpClosedNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolAndNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/globalCardinalityLowUpNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intAllEqualNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/setInNode.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

GlobalCardinalityLowUpClosedNode::GlobalCardinalityLowUpClosedNode(
    InvariantGraph& graph, std::vector<VarNodeId>&& x, std::vector<Int>&& cover,
    std::vector<Int>&& low, std::vector<Int>&& up, VarNodeId r)
    : ViolationInvariantNode(graph, {}, std::move(x), r),
      _cover(std::move(cover)),
      _low(std::move(low)),
      _up(std::move(up)) {}

GlobalCardinalityLowUpClosedNode::GlobalCardinalityLowUpClosedNode(
    InvariantGraph& graph, std::vector<VarNodeId>&& x, std::vector<Int>&& cover,
    std::vector<Int>&& low, std::vector<Int>&& up, bool shouldHold)
    : ViolationInvariantNode(graph, {}, std::move(x), shouldHold),
      _cover(std::move(cover)),
      _low(std::move(low)),
      _up(std::move(up)) {}

void GlobalCardinalityLowUpClosedNode::init(InvariantNodeId id) {
  ViolationInvariantNode::init(id);
  assert(
      !isReified() ||
      !invariantGraphConst().varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(outputVarNodeIds().size() <= 1);
  assert(std::ranges::all_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
}

void GlobalCardinalityLowUpClosedNode::updateState() {
  ViolationInvariantNode::updateState();
  if (staticInputVarNodeIds().empty() && _cover.empty()) {
    if (isReified()) {
      fixReified(true);
    } else if (!shouldHold()) {
      throw InconsistencyException(
          "GlobalCardinalityClosedNode::updateState neg: no inputs and empty "
          "cover");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (_cover.empty()) {
    if (isReified()) {
      fixReified(false);
    } else if (shouldHold()) {
      throw InconsistencyException(
          "GlobalCardinalityClosedNode::updateState: empty cover");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (isReified()) {
    return;
  }
  const SortedUniqueVector coveredVals(std::vector<Int>{_cover});
  if (shouldHold()) {
    for (const auto vId : staticInputVarNodeIds()) {
      invariantGraph().varNode(vId).domain()->removeAllValuesExcept(
          coveredVals);
    }
  }
}

bool GlobalCardinalityLowUpClosedNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE;
}

bool GlobalCardinalityLowUpClosedNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (!isReified() && shouldHold()) {
    invariantGraph().addInvariantNode(
        std::make_shared<GlobalCardinalityLowUpNode>(
            invariantGraph(), std::vector<VarNodeId>{staticInputVarNodeIds()},
            std::vector<Int>{_cover}, std::vector<Int>{_low},
            std::vector<Int>{_up}));
    return true;
  }

  std::vector<VarNodeId> violationVarNodeIds;
  violationVarNodeIds.reserve(staticInputVarNodeIds().size() + 1);

  for (VarNodeId inputId : staticInputVarNodeIds()) {
    violationVarNodeIds.emplace_back(invariantGraph().retrieveBoolVarNode());

    invariantGraph().addInvariantNode(std::make_shared<SetInNode>(
        invariantGraph(), inputId, std::vector<Int>(_cover),
        violationVarNodeIds.back()));
  }

  violationVarNodeIds.emplace_back(invariantGraph().retrieveBoolVarNode());

  invariantGraph().addInvariantNode(
      std::make_shared<GlobalCardinalityLowUpNode>(
          invariantGraph(), std::vector<VarNodeId>{staticInputVarNodeIds()},
          std::vector<Int>{_cover}, std::vector<Int>{_low},
          std::vector<Int>{_up}, violationVarNodeIds.back()));

  if (isReified()) {
    invariantGraph().addInvariantNode(std::make_shared<ArrayBoolAndNode>(
        invariantGraph(), std::move(violationVarNodeIds),
        reifiedViolationNodeId()));
  } else {
    invariantGraph().addInvariantNode(std::make_shared<ArrayBoolAndNode>(
        invariantGraph(), std::move(violationVarNodeIds), false));
  }

  return true;
}

void GlobalCardinalityLowUpClosedNode::registerOutputVars(propagation::SolverBase&, SolverMapping&) const {
  throw std::runtime_error("Not implemented");
}

void GlobalCardinalityLowUpClosedNode::registerNode(propagation::SolverBase&, SolverMapping&) const {
  throw std::runtime_error("Not implemented");
}

std::string GlobalCardinalityLowUpClosedNode::dotLangIdentifier() const {
  return "global_cardinality_low_up_closed";
}

}  // namespace atlantis::invariantgraph
