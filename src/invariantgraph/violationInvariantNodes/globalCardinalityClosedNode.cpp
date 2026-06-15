#include "atlantis/invariantgraph/violationInvariantNodes/globalCardinalityClosedNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNodes/globalCardinalityNode.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolAndNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intAllEqualNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intRelNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/setInNode.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

GlobalCardinalityClosedNode::GlobalCardinalityClosedNode(
    InvariantGraph& graph, std::vector<VarNodeId>&& inputs,
    std::vector<Int>&& cover, std::vector<VarNodeId>&& counts,
    const bool shouldHold)
    : ViolationInvariantNode(graph, std::move(counts), std::move(inputs),
                             shouldHold),
      _cover(std::move(cover)),
      _countOffsets(_cover.size(), 0) {}

GlobalCardinalityClosedNode::GlobalCardinalityClosedNode(
    InvariantGraph& graph, std::vector<VarNodeId>&& inputs,
    std::vector<Int>&& cover, std::vector<VarNodeId>&& counts,
    const VarNodeId r)
    : ViolationInvariantNode(graph, std::move(counts), std::move(inputs), r),
      _cover(std::move(cover)),
      _countOffsets(_cover.size(), 0) {}

void GlobalCardinalityClosedNode::init(const InvariantNodeId id) {
  ViolationInvariantNode::init(id);
  if (isReified()) {
    assert(!invariantGraphConst()
                .varNodeConst(outputVarNodeIds().front())
                .isIntVar());
    assert(std::ranges::all_of(
        outputVarNodeIds().begin() + 1, outputVarNodeIds().end(),
        [&](const VarNodeId vId) {
          return invariantGraphConst().varNodeConst(vId).isIntVar();
        }));
  } else {
    assert(std::ranges::all_of(outputVarNodeIds(), [&](const VarNodeId vId) {
      return invariantGraphConst().varNodeConst(vId).isIntVar();
    }));
  }
  assert(std::ranges::all_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
}

void GlobalCardinalityClosedNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  if (isReified()) {
    std::vector<ConstraintVarId> outputs(_cover.size(),
                                         ConstraintVarId{NULL_NODE_ID});
    for (size_t i = 0; i < _cover.size(); i++) {
      outputs[i] = outputVarNodeConst(i + 1).constraintVarId();
    }
    return constraintSolver().fzn_global_cardinality_closed_reif(
        toConstraintVarIds(invariantGraphConst(), staticInputVarNodeIds()),
        _cover, outputs, reifiedVarNodeConst().constraintVarId());
  }
  constraintSolver().fzn_global_cardinality_closed(
      toConstraintVarIds(invariantGraphConst(), staticInputVarNodeIds()),
      _cover, toConstraintVarIds(invariantGraphConst(), outputVarNodeIds()),
      shouldHold());
}

void GlobalCardinalityClosedNode::registerOutputVars(propagation::SolverBase&,
                                                     SolverMapping&) const {
  throw std::runtime_error("Not implemented");
}

void GlobalCardinalityClosedNode::updateState() {
  if (!isReified() && shouldHold()) {
    for (Int index = 0; index < static_cast<Int>(_cover.size()); ++index) {
      for (Int dupIndex = static_cast<Int>(_cover.size()) - 1; dupIndex > index;
           --dupIndex) {
        if (_cover[index] == _cover[dupIndex]) {
          _cover.erase(_cover.begin() + dupIndex);
          const VarNodeId duplicateNodeId = outputVarNodeIds().at(dupIndex);
          removeOutputAtIndex(dupIndex);
          invariantGraph().replaceVarNode(duplicateNodeId,
                                          outputVarNodeIds().at(index));
        }
      }
    }
  }

  // GCC can define the same output multiple times. Therefore, split all outputs
  // that are defined multiple times:
  postAllEqualOnReplacedVars(invariantGraph(), splitOutputVarNodes());

  ViolationInvariantNode::updateState();
  if (isReified()) {
    return;
  }

  if (!shouldHold()) {
    const bool allOverlaps =
        gccIsClosed(invariantGraphConst(), staticInputVarNodeIds(), _cover);
    if (!allOverlaps) {
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
    const auto bounds = gccBounds(
        invariantGraphConst(), staticInputVarNodeIds(), _cover, _countOffsets);
    assert(bounds.size() == _cover.size());
    assert(outputVarNodeIds().size() == _cover.size());
    for (size_t i = 0; i < bounds.size(); i++) {
      if (outputVarNodeConst(i).constDomain()->isDisjoint(bounds[i].first,
                                                          bounds[i].second)) {
        setState(InvariantNodeState::SUBSUMED);
        outputVarNode(i).tightenDomainType(
            outputVarNodeConst(i).constDomain()->isInterval()
                ? DomainType::DOM_RANGE
                : DomainType::DOM_DOMAIN);
      }
    }
    return;
  }

  const auto [varsToRemove, coverIndicesToRemove] = gccUpdateState(
      invariantGraphConst(), staticInputVarNodeIds(), _cover, _countOffsets);

  const Int outputIndexOffset =
      reifiedViolationNodeId() == NULL_NODE_ID ? 0 : 1;
  assert(outputIndexOffset == 0 ||
         outputVarNodeIds().front() == reifiedViolationNodeId());
  for (Int i = static_cast<Int>(coverIndicesToRemove->size()) - 1; i >= 0;
       --i) {
    _cover.erase(_cover.begin() + i);
    _countOffsets.erase(_countOffsets.begin() + i);
    removeOutputAtIndex(i + outputIndexOffset);
  }

  for (const VarNodeId vId : varsToRemove) {
    removeStaticInputVarNode(vId);
  }

  if (_cover.empty() || staticInputVarNodeIds().empty()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool GlobalCardinalityClosedNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE;
}

bool GlobalCardinalityClosedNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (!isReified() && shouldHold()) {
    invariantGraph().addInvariantNode(std::make_shared<GlobalCardinalityNode>(
        invariantGraph(), std::vector<VarNodeId>{staticInputVarNodeIds()},
        std::vector<Int>{_cover}, std::vector<VarNodeId>{outputVarNodeIds()}));
    return true;
  }

  std::vector<VarNodeId> violationVarNodeIds;
  violationVarNodeIds.reserve(outputVarNodeIds().size() +
                              staticInputVarNodeIds().size());

  std::vector<VarNodeId> intermediateOutputNodeIds;
  intermediateOutputNodeIds.reserve(outputVarNodeIds().size());

  // the first index holds the reified variable
  for (size_t i = isReified() ? 1 : 0; i < outputVarNodeIds().size(); ++i) {
    intermediateOutputNodeIds.emplace_back(invariantGraph().retrieveIntVarNode(
        std::make_shared<SearchDomain>(
            0, static_cast<Int>(staticInputVarNodeIds().size())),
        DomainType::DOM_NONE));

    violationVarNodeIds.emplace_back(invariantGraph().retrieveBoolVarNode());

    invariantGraph().addInvariantNode(std::make_shared<IntRelNode>(
        invariantGraph(), outputVarNodeIds()[i], RelationType::REL_TYPE_EQ,
        intermediateOutputNodeIds.back(), violationVarNodeIds.back()));
  }

  for (VarNodeId inputId : staticInputVarNodeIds()) {
    violationVarNodeIds.emplace_back(invariantGraph().retrieveBoolVarNode());

    invariantGraph().addInvariantNode(std::make_shared<SetInNode>(
        invariantGraph(), inputId, std::vector<Int>(_cover),
        violationVarNodeIds.back()));
  }

  invariantGraph().addInvariantNode(std::make_shared<GlobalCardinalityNode>(
      invariantGraph(), std::vector<VarNodeId>{staticInputVarNodeIds()},
      std::vector<Int>{_cover}, std::move(intermediateOutputNodeIds),
      std::move(_countOffsets)));

  if (isReified()) {
    invariantGraph().addInvariantNode(std::make_shared<ArrayBoolAndNode>(
        invariantGraph(), std::move(violationVarNodeIds),
        reifiedViolationNodeId()));
    return true;
  }
  invariantGraph().addInvariantNode(std::make_shared<ArrayBoolAndNode>(
      invariantGraph(), std::move(violationVarNodeIds), false));
  return true;
}

void GlobalCardinalityClosedNode::registerNode(propagation::SolverBase&,
                                               SolverMapping&) const {
  throw std::runtime_error("Not implemented");
}

std::string GlobalCardinalityClosedNode::dotLangIdentifier() const {
  return "global_cardinality_closed";
}

}  // namespace atlantis::invariantgraph
