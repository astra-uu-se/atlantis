#include "atlantis/invariantgraph/violationInvariantNodes/intAllEqualNode.hpp"

#include <algorithm>
#include <limits>
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
#include "atlantis/utils/domains.hpp"

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
    if (isReified()) {
      fixReified(true);
    } else if (!shouldHold()) {
      throw InconsistencyException(
          "IntAllEqualNode::updateState constraint is violated");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());
  for (const auto vId : staticInputVarNodeIds()) {
    const VarNode& vNode = invariantGraphConst().varNodeConst(vId);
    if (!vNode.isFixed()) {
      continue;
    }
    const Int val = vNode.lowerBound();
    if (_boundVal.has_value() && val != _boundVal.value()) {
      if (isReified()) {
        fixReified(false);
      } else if (shouldHold()) {
        throw InconsistencyException(
            "IntAllEqualNode::updateState constraint is violated");
      }

      setState(InvariantNodeState::SUBSUMED);
      return;
    }
    _boundVal.emplace(val);
    varsToRemove.emplace_back(vId);
  }

  if (_boundVal.has_value() && !isReified() && shouldHold()) {
    for (const auto vId : staticInputVarNodeIds()) {
      invariantGraph().varNode(vId).fixToValue(_boundVal.value());
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }

  for (const auto& vId : varsToRemove) {
    removeStaticInputVarNode(vId);
  }

  if (!_boundVal.has_value()) {
    Int overlapLb = std::numeric_limits<Int>::min();
    Int overlapUb = std::numeric_limits<Int>::max();
    for (const auto& vId : staticInputVarNodeIds()) {
      const VarNode& vNode = invariantGraphConst().varNodeConst(vId);
      overlapLb = std::max(overlapLb, vNode.lowerBound());
      overlapUb = std::min(overlapUb, vNode.upperBound());
    }
    if (overlapLb > overlapUb) {
      if (isReified()) {
        fixReified(false);
      } else if (shouldHold()) {
        throw InconsistencyException(
            "IntAllEqualNode::updateState constraint is violated");
      }
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
    if (overlapLb == overlapUb && !isReified() && shouldHold()) {
      _boundVal.emplace(overlapLb);
      for (const auto vId : staticInputVarNodeIds()) {
        invariantGraph().varNode(vId).fixToValue(_boundVal.value());
      }
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
    if (overlapLb + 1 < overlapUb) {
      SearchDomain overlap(overlapLb, overlapUb);
      try {
        for (const auto vId : staticInputVarNodeIds()) {
          overlap.intersect(
              *invariantGraphConst().varNodeConst(vId).constDomain());
        }
      } catch (const InconsistencyException&) {
        if (isReified()) {
          fixReified(false);
        } else if (shouldHold()) {
          throw;
        }
        setState(InvariantNodeState::SUBSUMED);
        return;
      }
    }
  }

  if (staticInputVarNodeIds().empty()) {
    if (isReified()) {
      fixReified(true);
    } else if (!shouldHold()) {
      throw InconsistencyException(
          "IntAllEqualNode::updateState constraint is violated");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (staticInputVarNodeIds().size() == 1 && _boundVal.has_value() &&
      !isReified()) {
    assert(!shouldHold());
    auto& vNode = invariantGraph().varNode(staticInputVarNodeIds().front());
    vNode.removeValueAndTightenDomainType(_boundVal.value());
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool IntAllEqualNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE && !isReified() &&
         !shouldHold() && staticInputVarNodeIds().size() <= 2 &&
         !_boundVal.has_value();
}

bool IntAllEqualNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  invariantGraph().addInvariantNode(std::make_shared<AllDifferentNode>(
      invariantGraph(), std::vector<VarNodeId>{staticInputVarNodeIds()}));
  return true;
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
              solver(), _intermediate, staticInputVarNodeIds().size()));
        } else {
          setViolationVarId(solver().makeIntView<propagation::NotEqualConst>(
              solver(), _intermediate, staticInputVarNodeIds().size()));
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
      staticInputVarNodeIds(), std::back_inserter(inputVarIds),
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
