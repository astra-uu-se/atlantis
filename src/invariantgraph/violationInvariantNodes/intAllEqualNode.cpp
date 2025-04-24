#include "atlantis/invariantgraph/violationInvariantNodes/intAllEqualNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/allDifferentNode.hpp"
#include "atlantis/propagation/invariants/countConst.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/equalConst.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/propagation/violationInvariants/allDifferent.hpp"
#include "atlantis/propagation/violationInvariants/equal.hpp"
#include "atlantis/propagation/violationInvariants/notEqual.hpp"

namespace atlantis::invariantgraph {

IntAllEqualNode::IntAllEqualNode(InvariantGraph& graph, VarNodeId a,
                                 VarNodeId b, VarNodeId r, bool breaksCycle)
    : IntAllEqualNode(graph, std::vector<VarNodeId>{a, b}, r, breaksCycle) {}

IntAllEqualNode::IntAllEqualNode(InvariantGraph& graph, VarNodeId a,
                                 VarNodeId b, bool shouldHold, bool breaksCycle)
    : IntAllEqualNode(graph, std::vector<VarNodeId>{a, b}, shouldHold,
                      breaksCycle) {}

IntAllEqualNode::IntAllEqualNode(InvariantGraph& graph,
                                 std::vector<VarNodeId>&& vars, VarNodeId r,
                                 bool breaksCycle)
    : ViolationInvariantNode(graph, std::move(vars), r),
      _breaksCycle(breaksCycle) {}

IntAllEqualNode::IntAllEqualNode(InvariantGraph& graph,
                                 std::vector<VarNodeId>&& vars, bool shouldHold,
                                 bool breaksCycle)
    : ViolationInvariantNode(graph, std::move(vars), shouldHold),
      _breaksCycle(breaksCycle) {}

void IntAllEqualNode::init(InvariantNodeId id) {
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

void IntAllEqualNode::updateState() {
  ViolationInvariantNode::updateState();
  if (staticInputVarNodeIds().size() < 2 && !_boundVal.has_value()) {
    if (!isReified() && !shouldHold()) {
      throw InconsistencyException(
          "IntAllEqualNode::updateState constraint is violated");
    }
    if (isReified()) {
      fixReified(true);
    }
    setState(InvariantNodeState::SUBSUMED);
  }
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());
  for (const auto vId : staticInputVarNodeIds()) {
    VarNode& vNode = invariantGraph().varNode(vId);
    if (!vNode.isFixed()) {
      continue;
    }
    const Int val = vNode.lowerBound();
    if (_boundVal.has_value() && val != _boundVal.value()) {
      if (!isReified() && shouldHold()) {
        throw InconsistencyException(
            "IntAllEqualNode::updateState constraint is violated");
      }
      if (isReified()) {
        fixReified(false);
      }
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
    _boundVal.emplace(val);
    varsToRemove.emplace_back(vId);
  }

  if (_boundVal.has_value() && !isReified() && shouldHold()) {
    for (const auto vId : varsToRemove) {
      invariantGraph().varNode(vId).fixToValue(_boundVal.value());
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }

  for (const auto& vId : varsToRemove) {
    removeStaticInputVarNode(vId);
  }

  if (staticInputVarNodeIds().empty()) {
    if (isReified()) {
      fixReified(true);
    }
    setState(InvariantNodeState::SUBSUMED);
  }
  if (staticInputVarNodeIds().size() == 1 && _boundVal.has_value() &&
      !isReified()) {
    assert(!shouldHold());
    invariantGraph()
        .varNode(staticInputVarNodeIds().front())
        .removeValue(_boundVal.value());
    setState(InvariantNodeState::SUBSUMED);
  }
}

void IntAllEqualNode::registerOutputVars() {
  assert(!staticInputVarNodeIds().empty());
  if (violationVarId() == propagation::NULL_ID) {
    if (_boundVal.has_value()) {
      if (staticInputVarNodeIds().size() == 1) {
        assert(isReified());
        assert(invariantGraphConst().varId(staticInputVarNodeIds().front()) !=
               propagation::NULL_ID);
        setViolationVarId(solver().makeIntView<propagation::EqualConst>(
            solver(),
            invariantGraphConst().varId(staticInputVarNodeIds().front()),
            _boundVal.value()));
      } else if (_intermediate == propagation::NULL_ID) {
        _intermediate = solver().makeIntVar(0, 0, 0);
        if (shouldHold()) {
          setViolationVarId(solver().makeIntView<propagation::EqualConst>(
              solver(),
              invariantGraphConst().varId(staticInputVarNodeIds().front()),
              staticInputVarNodeIds().size()));
        } else {
          setViolationVarId(solver().makeIntView<propagation::NotEqualConst>(
              solver(),
              invariantGraphConst().varId(staticInputVarNodeIds().front()),
              staticInputVarNodeIds().size()));
        }
      }
    } else if (staticInputVarNodeIds().size() == 2) {
      registerViolation();
    } else if (_intermediate == propagation::NULL_ID) {
      _intermediate = solver().makeIntVar(0, 0, 0);
      if (shouldHold()) {
        setViolationVarId(solver().makeIntView<propagation::EqualConst>(
            solver(), _intermediate, staticInputVarNodeIds().size() - 1));
      } else {
        assert(!isReified());
        setViolationVarId(solver().makeIntView<propagation::NotEqualConst>(
            solver(), _intermediate, staticInputVarNodeIds().size() - 1));
      }
    }
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).varId() !=
               propagation::NULL_ID;
      }));
}

void IntAllEqualNode::registerNode() {
  assert(violationVarId() != propagation::NULL_ID);

  if (_boundVal.has_value() && staticInputVarNodeIds().size() <= 1) {
    return;
  }

  assert(staticInputVarNodeIds().size() >= 2);

  std::vector<propagation::VarViewId> inputVarIds;
  inputVarIds.reserve(staticInputVarNodeIds().size());
  std::ranges::transform(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      std::back_inserter(inputVarIds),
      [&](const auto& id) { return invariantGraph().varId(id); });

  if (_boundVal.has_value()) {
    assert(_intermediate != propagation::NULL_ID);
    assert(_intermediate.isVar());
    solver().makeInvariant<propagation::CountConst>(
        solver(), _intermediate, _boundVal.value(), std::move(inputVarIds));
    return;
  }

  if (inputVarIds.size() == 2) {
    assert(violationVarId().isVar());
    assert(_intermediate == propagation::NULL_ID);
    if (shouldHold()) {
      solver().makeViolationInvariant<propagation::Equal>(
          solver(), violationVarId(), inputVarIds.front(), inputVarIds.back());
    } else {
      solver().makeViolationInvariant<propagation::NotEqual>(
          solver(), violationVarId(), inputVarIds.front(), inputVarIds.back());
    }
    return;
  }

  assert(_intermediate != propagation::NULL_ID);
  assert(_intermediate.isVar());

  solver().makeViolationInvariant<propagation::AllDifferent>(
      solver(), _intermediate, std::move(inputVarIds));
}

std::string IntAllEqualNode::dotLangIdentifier() const {
  return "int_all_equal";
}

}  // namespace atlantis::invariantgraph
