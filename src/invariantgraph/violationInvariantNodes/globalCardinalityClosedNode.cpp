#include "atlantis/invariantgraph/violationInvariantNodes/globalCardinalityClosedNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNodes/globalCardinalityNode.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolAndNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intAllEqualNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/setInNode.hpp"
#include "atlantis/utils/domains.hpp"
#include "fznparser/except.hpp"

namespace atlantis::invariantgraph {

GlobalCardinalityClosedNode::GlobalCardinalityClosedNode(
    InvariantGraph& graph, std::vector<VarNodeId>&& inputs,
    std::vector<Int>&& cover, std::vector<VarNodeId>&& counts, bool shouldHold)
    : ViolationInvariantNode(graph, std::move(counts), std::move(inputs),
                             shouldHold),
      _cover(std::move(cover)) {}

GlobalCardinalityClosedNode::GlobalCardinalityClosedNode(
    InvariantGraph& graph, std::vector<VarNodeId>&& inputs,
    std::vector<Int>&& cover, std::vector<VarNodeId>&& counts, VarNodeId r)
    : ViolationInvariantNode(graph, std::move(counts), std::move(inputs), r),
      _cover(std::move(cover)) {}

void GlobalCardinalityClosedNode::init(InvariantNodeId id) {
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

void GlobalCardinalityClosedNode::registerOutputVars() {
  throw std::runtime_error("Not implemented");
}

void GlobalCardinalityClosedNode::updateState() {
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

    invariantGraph().addInvariantNode(std::make_shared<IntAllEqualNode>(
        invariantGraph(), outputVarNodeIds()[i],
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
      std::vector<Int>{_cover}, std::move(intermediateOutputNodeIds)));

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

void GlobalCardinalityClosedNode::registerNode() {
  throw std::runtime_error("Not implemented");
}

std::string GlobalCardinalityClosedNode::dotLangIdentifier() const {
  return "global_cardinality_closed";
}

}  // namespace atlantis::invariantgraph
