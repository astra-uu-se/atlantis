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
    InvariantGraph& graph, std::vector<std::shared_ptr<VarNode>>&& inputs,
    std::vector<Int>&& cover, std::vector<std::shared_ptr<VarNode>>&& counts,
    const bool shouldHold)
    : ViolationInvariantNode(graph, std::move(counts), std::move(inputs),
                             shouldHold),
      _cover(std::move(cover)),
      _countOffsets(_cover.size(), 0) {}

GlobalCardinalityClosedNode::GlobalCardinalityClosedNode(
    InvariantGraph& graph, std::vector<std::shared_ptr<VarNode>>&& inputs,
    std::vector<Int>&& cover, std::vector<std::shared_ptr<VarNode>>&& counts,
    const VarNode& r)
    : ViolationInvariantNode(graph, std::move(counts), std::move(inputs), r),
      _cover(std::move(cover)),
      _countOffsets(_cover.size(), 0) {}

void GlobalCardinalityClosedNode::init() {
  ViolationInvariantNode::init();
  if (isReified()) {
    assert(!invariantGraphConst()
                .varNodeConst(outputVarNodes().front())
                .isIntVar());
    assert(std::ranges::all_of(
        outputVarNodes().begin() + 1, outputVarNodes().end(),
        [&](const std::shared_ptr<VarNode>& vId) { return vId.isIntVar(); }));
  } else {
    assert(std::ranges::all_of(
        outputVarNodes(),
        [&](const std::shared_ptr<VarNode>& vId) { return vId.isIntVar(); }));
  }
  assert(std::ranges::all_of(
      staticInputVarNodes().begin(), staticInputVarNodes().end(),
      [&](const std::shared_ptr<VarNode>& vId) { return vId.isIntVar(); }));
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
        toConstraintVarIds(invariantGraphConst(), staticInputVarNodes()),
        _cover, outputs, reifiedVarNodeConst().constraintVarId());
  }
  constraintSolver().fzn_global_cardinality_closed(
      toConstraintVarIds(invariantGraphConst(), staticInputVarNodes()), _cover,
      toConstraintVarIds(invariantGraphConst(), outputVarNodes()),
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
          _countOffsets[index] += _countOffsets[dupIndex];

          _cover.erase(_cover.begin() + dupIndex);
          _countOffsets.erase(_countOffsets.begin() + dupIndex);

          const std::shared_ptr<VarNode>& duplicateNode =
              outputVarNodes().at(dupIndex);
          removeOutputAtIndex(dupIndex);
          invariantGraph().replaceVarNode(duplicateNode,
                                          outputVarNodes().at(index));
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
        gccIsClosed(invariantGraphConst(), staticInputVarNodes(), _cover);
    if (!allOverlaps) {
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
    const auto bounds = gccBounds(invariantGraphConst(), staticInputVarNodes(),
                                  _cover, _countOffsets);
    assert(bounds.size() == _cover.size());
    assert(outputVarNodes().size() == _cover.size());
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
      invariantGraphConst(), staticInputVarNodes(), _cover, _countOffsets);

  const Int outputIndexOffset = reifiedViolationNode() == NULL_NODE_ID ? 0 : 1;
  assert(outputIndexOffset == 0 ||
         outputVarNodes().front() == reifiedViolationNode());
  for (Int i = static_cast<Int>(coverIndicesToRemove->size()) - 1; i >= 0;
       --i) {
    _cover.erase(_cover.begin() + i);
    _countOffsets.erase(_countOffsets.begin() + i);
    removeOutputAtIndex(i + outputIndexOffset);
  }

  for (const auto& var : varsToRemove) {
    removeStaticInputVarNode(var);
  }

  if (_cover.empty() || staticInputVarNodes().empty()) {
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
        invariantGraph(),
        std::vector<std::shared_ptr<VarNode>>{staticInputVarNodes()},
        std::vector<Int>{_cover},
        std::vector<std::shared_ptr<VarNode>>{outputVarNodes()},
        std::move(_countOffsets)));
    return true;
  }

  std::vector<std::shared_ptr<VarNode>> violationVarNodeIds;
  violationVarNodeIds.reserve(outputVarNodes().size() +
                              staticInputVarNodes().size());

  std::vector<std::shared_ptr<VarNode>> intermediateOutputNodeIds;
  intermediateOutputNodeIds.reserve(outputVarNodes().size());

  // the first index holds the reified variable
  for (size_t i = isReified() ? 1 : 0; i < outputVarNodes().size(); ++i) {
    intermediateOutputNodeIds.emplace_back(invariantGraph().retrieveIntVarNode(
        std::make_shared<SearchDomain>(
            0, static_cast<Int>(staticInputVarNodes().size())),
        DomainType::DOM_NONE));

    violationVarNodeIds.emplace_back(invariantGraph().retrieveBoolVarNode());

    invariantGraph().addInvariantNode(std::make_shared<IntRelNode>(
        invariantGraph(), outputVarNodes()[i], RelationType::REL_TYPE_EQ,
        intermediateOutputNodeIds.back(), violationVarNodeIds.back()));
  }

  for (const auto& input : staticInputVarNodes()) {
    violationVarNodeIds.emplace_back(invariantGraph().retrieveBoolVarNode());

    invariantGraph().addInvariantNode(std::make_shared<SetInNode>(
        invariantGraph(), input, std::vector<Int>(_cover),
        violationVarNodeIds.back()));
  }

  invariantGraph().addInvariantNode(std::make_shared<GlobalCardinalityNode>(
      invariantGraph(),
      std::vector<std::shared_ptr<VarNode>>{staticInputVarNodes()},
      std::vector<Int>{_cover}, std::move(intermediateOutputNodeIds),
      std::move(_countOffsets)));

  if (isReified()) {
    invariantGraph().addInvariantNode(std::make_shared<ArrayBoolAndNode>(
        invariantGraph(), std::move(violationVarNodeIds),
        reifiedViolationNode()));
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
