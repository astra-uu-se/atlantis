#include "atlantis/invariantgraph/violationInvariantNodes/globalCardinalityLowUpNode.hpp"

#include <algorithm>
#include <stack>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/propagation/violationInvariants/globalCardinalityLowUp.hpp"

namespace atlantis::invariantgraph {

GlobalCardinalityLowUpNode::GlobalCardinalityLowUpNode(
    InvariantGraph& graph, std::vector<VarNodeId>&& x, std::vector<Int>&& cover,
    std::vector<Int>&& low, std::vector<Int>&& up, VarNodeId r)
    : ViolationInvariantNode(graph, {}, std::move(x), r),
      _cover(std::move(cover)),
      _low(std::move(low)),
      _up(std::move(up)) {}

GlobalCardinalityLowUpNode::GlobalCardinalityLowUpNode(
    InvariantGraph& graph, std::vector<VarNodeId>&& x, std::vector<Int>&& cover,
    std::vector<Int>&& low, std::vector<Int>&& up, bool shouldHold)
    : ViolationInvariantNode(graph, {}, std::move(x), shouldHold),
      _cover(std::move(cover)),
      _low(std::move(low)),
      _up(std::move(up)) {}

void GlobalCardinalityLowUpNode::init(InvariantNodeId id) {
  ViolationInvariantNode::init(id);
  assert(
      !isReified() ||
      !invariantGraphConst().varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(std::ranges::all_of(
      outputVarNodeIds().begin() + (isReified() ? 1 : 0),
      outputVarNodeIds().end(), [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
  assert(std::ranges::all_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
}

void GlobalCardinalityLowUpNode::verifyCover() {
  for (Int i = static_cast<Int>(_cover.size()) - 1; i >= 0; --i) {
    _low[i] = std::max<Int>(Int{0}, _low[i]);
    if (_low[i] > _up[i]) {
      if (isReified()) {
        fixReified(false);
      } else if (shouldHold()) {
        throw InconsistencyException(
            "GlobalCardinalityLowUpNode::updateState: low[" +
            std::to_string(i) + "] > up[" + std::to_string(i) + "] (" +
            std::to_string(_low[i]) + " > " + std::to_string(_up[i]) + ").");
      }
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
  }
}

void GlobalCardinalityLowUpNode::propagate() {
  if (isReified() || !shouldHold()) {
    return;
  }
  std::vector<std::vector<size_t>> supportedInputs(_cover.size());
  std::vector<std::vector<size_t>> supportedCovers(
      staticInputVarNodeIds().size());
  for (size_t inputIndex = 0; inputIndex < staticInputVarNodeIds().size();
       inputIndex++) {
    const auto& var =
        invariantGraphConst().varNodeConst(staticInputVarNodeIds()[inputIndex]);
    if (var.isFixed()) {
      for (size_t coverIndex = 0; coverIndex < _cover.size(); ++coverIndex) {
        if (var.lowerBound() == _cover[coverIndex]) {
          --_low[coverIndex];
          --_up[coverIndex];
        }
      }
    } else {
      for (size_t coverIndex = 0; coverIndex < _cover.size(); ++coverIndex) {
        if (var.inDomain(_cover[coverIndex])) {
          supportedInputs[coverIndex].emplace_back(inputIndex);
          supportedCovers[inputIndex].emplace_back(coverIndex);
        }
      }
    }
  }

  std::vector<bool> onStack(_cover.size(), true);
  std::stack<size_t> stack;
  for (size_t coverIndex = 0; coverIndex < _cover.size(); ++coverIndex) {
    stack.push(coverIndex);
  }

  while (!stack.empty()) {
    const size_t coverIndex = stack.top();
    stack.pop();
    if (_low[coverIndex] ==
        static_cast<Int>(supportedInputs[coverIndex].size())) {
      for (const size_t inputIndex : supportedInputs[coverIndex]) {
        auto& vNode =
            invariantGraph().varNode(staticInputVarNodeIds()[inputIndex]);
        vNode.fixToValue(_cover[coverIndex]);
        for (const size_t otherCover : supportedCovers[inputIndex]) {
          if (otherCover != coverIndex) {
            removeFirstOccurrence(supportedInputs[otherCover], inputIndex);
            if (!onStack[otherCover]) {
              stack.push(otherCover);
              onStack[otherCover] = true;
            }
          }
        }
        supportedInputs[coverIndex].clear();
      }
    } else if (_up[coverIndex] == 0) {
      for (const size_t inputIndex : supportedInputs[coverIndex]) {
        auto& vNode =
            invariantGraph().varNode(staticInputVarNodeIds()[inputIndex]);
        vNode.removeValue(_cover[coverIndex]);
        removeFirstOccurrence(supportedCovers[inputIndex], coverIndex);
        if (vNode.isFixed() && !supportedCovers[inputIndex].empty()) {
          assert(supportedCovers[inputIndex].size() == 1);
          const size_t otherCover = supportedCovers[inputIndex].front();
          assert(otherCover != coverIndex);
          --_low[otherCover];
          --_up[otherCover];
          removeFirstOccurrence(supportedInputs[otherCover], inputIndex);
          if (!onStack[otherCover]) {
            stack.push(otherCover);
            onStack[otherCover] = true;
          }
        }
      }
      supportedInputs[coverIndex].clear();
    }
    onStack[coverIndex] = false;
  }

  verifyCover();

  for (Int i = static_cast<Int>(_cover.size()) - 1; i >= 0; --i) {
    if (supportedInputs[i].empty()) {
      _cover.erase(_cover.begin() + i);
      _low.erase(_low.begin() + i);
      _up.erase(_up.begin() + i);
    } else {
      _low[i] = std::max<Int>(Int{0}, _low[i]);
    }
  }

  std::vector<VarNodeId> inputsToRemove;
  inputsToRemove.reserve(_cover.size());
  for (Int i = static_cast<Int>(staticInputVarNodeIds().size()) - 1; i >= 0;
       --i) {
    if (supportedCovers[i].empty()) {
      inputsToRemove.emplace_back(staticInputVarNodeIds()[i]);
    }
  }
  for (const auto& input : inputsToRemove) {
    removeStaticInputVarNode(input);
  }
}

void GlobalCardinalityLowUpNode::updateState() {
  ViolationInvariantNode::updateState();
  if (_cover.empty()) {
    if (isReified()) {
      fixReified(true);
    } else if (!shouldHold()) {
      throw InconsistencyException(
          "GlobalCardinalityLowUpNode neg: empty domain.");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }

  for (Int i = 0; i < static_cast<Int>(_cover.size()); i++) {
    for (Int j = static_cast<Int>(_cover.size()) - 1; j > i; --j) {
      if (_cover[i] == _cover[j]) {
        _low[i] = std::max(_low[i], _low[j]);
        _up[i] = std::min(_up[i], _up[j]);
        _cover.erase(_cover.begin() + j);
        _low.erase(_low.begin() + j);
        _up.erase(_up.begin() + j);
      }
    }
  }

  if (staticInputVarNodeIds().empty()) {
    bool satisfied = true;
    for (size_t i = 0; i < _cover.size(); ++i) {
      satisfied &= _low[i] <= 0 && 0 <= _up[i];
      if (!satisfied) {
        break;
      }
    }
    if (isReified()) {
      fixReified(satisfied);
    } else if (shouldHold() != satisfied) {
      throw InconsistencyException(
          "GlobalCardinalityLowUpNode neg: no inputs and bad low up.");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }

  verifyCover();
  propagate();

  if (_cover.empty()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

void GlobalCardinalityLowUpNode::registerOutputVars() {
  if (violationVarId() == propagation::NULL_ID) {
    if (!shouldHold()) {
      _intermediate = solver().makeIntVar(
          0, 0, static_cast<Int>(staticInputVarNodeIds().size()));
      setViolationVarId(solver().makeIntView<propagation::NotEqualConst>(
          solver(), _intermediate, 0));
    } else {
      registerViolation();
    }
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).varId() !=
               propagation::NULL_ID;
      }));
}

void GlobalCardinalityLowUpNode::registerNode() {
  std::vector<propagation::VarViewId> inputVarIds;
  assert(violationVarId() != propagation::NULL_ID);
  assert(shouldHold() || _intermediate != propagation::NULL_ID);
  assert(shouldHold() ? violationVarId().isVar() : _intermediate.isVar());

  std::ranges::transform(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      std::back_inserter(inputVarIds),
      [&](const auto& id) { return invariantGraph().varId(id); });

  if (shouldHold()) {
    solver().makeInvariant<propagation::GlobalCardinalityLowUp>(
        solver(), violationVarId(), std::move(inputVarIds),
        std::vector<Int>(_cover), std::vector<Int>(_low),
        std::vector<Int>(_up));
  } else {
    solver().makeInvariant<propagation::GlobalCardinalityLowUp>(
        solver(), _intermediate, std::move(inputVarIds),
        std::vector<Int>(_cover), std::vector<Int>(_low),
        std::vector<Int>(_up));
  }
}

std::string GlobalCardinalityLowUpNode::dotLangIdentifier() const {
  return "global_cardinality_low_up";
}

}  // namespace atlantis::invariantgraph
