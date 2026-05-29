#include "atlantis/invariantgraph/violationInvariantNodes/globalCardinalityLowUpClosedNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolAndNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/globalCardinalityLowUpNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/setInNode.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

GlobalCardinalityLowUpClosedNode::GlobalCardinalityLowUpClosedNode(
    InvariantGraph& graph, std::vector<VarNodeId>&& x, std::vector<Int>&& cover,
    std::vector<Int>&& low, std::vector<Int>&& up, const VarNodeId r)
    : ViolationInvariantNode(graph, {}, std::move(x), r),
      _cover(std::move(cover)),
      _low(std::move(low)),
      _up(std::move(up)) {}

GlobalCardinalityLowUpClosedNode::GlobalCardinalityLowUpClosedNode(
    InvariantGraph& graph, std::vector<VarNodeId>&& x, std::vector<Int>&& cover,
    std::vector<Int>&& low, std::vector<Int>&& up, const bool shouldHold)
    : ViolationInvariantNode(graph, {}, std::move(x), shouldHold),
      _cover(std::move(cover)),
      _low(std::move(low)),
      _up(std::move(up)) {}

void GlobalCardinalityLowUpClosedNode::init(const InvariantNodeId id) {
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

void GlobalCardinalityLowUpClosedNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  if (isReified()) {
    return constraintSolver().fzn_global_cardinality_low_up_closed_reif(toConstraintVarIds(invariantGraphConst(), staticInputVarNodeIds()), _cover, _low, _up, reifiedVarNodeConst().constraintVarId());
  }
  constraintSolver().fzn_global_cardinality_low_up_closed(toConstraintVarIds(invariantGraphConst(), staticInputVarNodeIds()), _cover, _low, _up, shouldHold());
}

void GlobalCardinalityLowUpClosedNode::updateState() {
  // GCC can define the same output multiple times. Therefore, split all outputs
  // that are defined multiple times:
  postAllEqualOnReplacedVars(invariantGraph(), splitOutputVarNodes());

  ViolationInvariantNode::updateState();

  if (isReified() || !shouldHold()) {
    return;
  }

  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());

  std::vector<bool> coverIntersectsDomains(_cover.size(), false);

  for (const VarNodeId vId : staticInputVarNodeIds()) {
    bool domainIntersectsCover = false;
    for (size_t coverIndex = 0; coverIndex < _cover.size(); ++coverIndex) {
      if (varNodeConst(vId).isFixed()) {
        if (varNodeConst(vId).lowerBound() == _cover[coverIndex]) {
          --_low[coverIndex];
          --_up[coverIndex];
        }
        varsToRemove.emplace_back(vId);
        break;
      }
      if (varNodeConst(vId).inDomain(_cover[coverIndex])) {
        coverIntersectsDomains[coverIndex] = true;
        domainIntersectsCover = true;
      }
    }
    if (!domainIntersectsCover) {
      varsToRemove.emplace_back(vId);
    }
  }

  for (Int i = 0; i < static_cast<Int>(_cover.size()); ++i) {
    if (!coverIntersectsDomains[i]) {
      _cover.erase(_cover.begin() + i);
      _low.erase(_low.begin() + i);
      _up.erase(_up.begin() + i);
    }
  }

  for (const VarNodeId vId : varsToRemove) {
    removeStaticInputVarNode(vId);
  }

  if (_cover.empty() || staticInputVarNodeIds().empty()) {
    setState(InvariantNodeState::SUBSUMED);
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

void GlobalCardinalityLowUpClosedNode::registerOutputVars(
    propagation::SolverBase&, SolverMapping&) const {
  throw std::runtime_error("Not implemented");
}

void GlobalCardinalityLowUpClosedNode::registerNode(propagation::SolverBase&,
                                                    SolverMapping&) const {
  throw std::runtime_error("Not implemented");
}

std::string GlobalCardinalityLowUpClosedNode::dotLangIdentifier() const {
  return "global_cardinality_low_up_closed";
}

}  // namespace atlantis::invariantgraph
