#include "atlantis/invariantgraph/violationInvariantNodes/boolAllEqualNode.hpp"

#include <algorithm>
#include <boost/fiber/algo/algorithm.hpp>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/views/boolNotNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolAndNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolOrNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolXorNode.hpp"
#include "atlantis/propagation/invariants/boolXor.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/propagation/violationInvariants/boolAllEqual.hpp"
#include "atlantis/propagation/violationInvariants/boolEqual.hpp"

namespace atlantis::invariantgraph {

BoolAllEqualNode::BoolAllEqualNode(InvariantGraph& graph, VarNodeId a,
                                   VarNodeId b, VarNodeId r, bool breaksCycle)
    : BoolAllEqualNode(graph, std::vector<VarNodeId>{a, b}, r, breaksCycle) {}

BoolAllEqualNode::BoolAllEqualNode(InvariantGraph& graph, VarNodeId a,
                                   VarNodeId b, bool shouldHold,
                                   bool breaksCycle)
    : BoolAllEqualNode(graph, std::vector<VarNodeId>{a, b}, shouldHold,
                       breaksCycle) {}

BoolAllEqualNode::BoolAllEqualNode(InvariantGraph& graph,
                                   std::vector<VarNodeId>&& vars, VarNodeId r,
                                   bool breaksCycle)
    : ViolationInvariantNode(graph, std::move(vars), r),
      _breaksCycle(breaksCycle) {}

BoolAllEqualNode::BoolAllEqualNode(InvariantGraph& graph,
                                   std::vector<VarNodeId>&& vars,
                                   bool shouldHold, bool breaksCycle)
    : ViolationInvariantNode(graph, std::move(vars), shouldHold),
      _breaksCycle(breaksCycle) {}

bool BoolAllEqualNode::isFixed() const { return _dom < 2; }

bool BoolAllEqualNode::inDomain(bool val) const {
  return val ? holdsTrue() : holdsFalse();
}

bool BoolAllEqualNode::holdsTrue() const { return _dom > 0; }

bool BoolAllEqualNode::holdsFalse() const { return _dom != 1; }

void BoolAllEqualNode::fixToVal(bool val) { _dom = val ? 1 : 0; }

void BoolAllEqualNode::init(InvariantNodeId id) {
  ViolationInvariantNode::init(id);
  assert(
      !isReified() ||
      !invariantGraphConst().varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(std::ranges::none_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
}

void BoolAllEqualNode::updateState() {
  ViolationInvariantNode::updateState();

  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());

  for (const auto vId : staticInputVarNodeIds()) {
    VarNode& vNode = invariantGraph().varNode(vId);
    if (!vNode.isFixed()) {
      if (!isReified() && shouldHold() && isFixed()) {
        vNode.fixToValue(holdsTrue());
        varsToRemove.emplace_back(vId);
      }
    } else {
      const bool val = vNode.inDomain(bool{true});
      if (inDomain(val)) {
        if (!isFixed()) {
          fixToVal(val);
        }
      } else if (isReified()) {
        fixReified(false);
        setState(InvariantNodeState::SUBSUMED);
        return;
      } else if (!shouldHold()) {
        setState(InvariantNodeState::SUBSUMED);
        return;
      } else {
        throw InconsistencyException(
            "BoolAllEqualNode::updateState constraint is violated");
      }
      varsToRemove.emplace_back(vId);
    }
  }

  for (const auto vId : varsToRemove) {
    removeStaticInputVarNode(vId);
  }

  if (staticInputVarNodeIds().empty()) {
    if (isReified()) {
      fixReified(true);
      setState(InvariantNodeState::SUBSUMED);
    } else if (shouldHold()) {
      setState(InvariantNodeState::SUBSUMED);
    } else {
      throw InconsistencyException(
          "BoolAllEqualNode::updateState constraint is Violated");
    }
  } else if (staticInputVarNodeIds().size() == 1) {
    auto& vNode = invariantGraph().varNode(staticInputVarNodeIds().front());
    if (!isReified() && !shouldHold()) {
      if (isFixed()) {
        vNode.fixToValue(!holdsTrue());
        setState(InvariantNodeState::SUBSUMED);
      } else {
        throw InconsistencyException(
            "BoolAllEqualNode::updateState constraint is violated");
      }
    } else if (!isReified()) {
      if (isFixed()) {
        vNode.fixToValue(holdsTrue());
      }
      setState(InvariantNodeState::SUBSUMED);
    } else if (!isFixed()) {
      fixReified(true);
      setState(InvariantNodeState::SUBSUMED);
    }
  }
}

bool BoolAllEqualNode::canBeReplaced() const {
  if (state() != InvariantNodeState::ACTIVE || _breaksCycle) {
    return false;
  }
  if (isFixed()) {
    return true;
  }
  return !isReified() && (shouldHold() || staticInputVarNodeIds().size() == 2);
}

bool BoolAllEqualNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (isFixed()) {
    if (shouldHold()) {
      // node is reified
      if (holdsTrue()) {
        // all fixed vars takes value true
        if (isReified()) {
          // The reified var is true iff all unfixed vars takes value true
          invariantGraph().addInvariantNode(std::make_shared<ArrayBoolAndNode>(
              invariantGraph(), std::vector<VarNodeId>{staticInputVarNodeIds()},
              reifiedViolationNodeId()));
        } else {
          // The constraint holds iff all unfixed vars takes value true
          invariantGraph().addInvariantNode(std::make_shared<ArrayBoolAndNode>(
              invariantGraph(), std::vector<VarNodeId>{staticInputVarNodeIds()},
              true));
        }
      } else if (isReified()) {
        // The reified var is true iff all unfixed vars takes value false
        const VarNodeId invReif = invariantGraph().retrieveBoolVarNode();
        invariantGraph().addInvariantNode(std::make_shared<BoolNotNode>(
            invariantGraph(), reifiedViolationNodeId(), invReif));
        invariantGraph().addInvariantNode(std::make_shared<ArrayBoolOrNode>(
            invariantGraph(), std::vector<VarNodeId>{staticInputVarNodeIds()},
            invReif));
      } else {
        // The constraint holds iff all unfixed vars takes value false
        invariantGraph().addInvariantNode(std::make_shared<ArrayBoolOrNode>(
            invariantGraph(), std::vector<VarNodeId>{staticInputVarNodeIds()},
            false));
      }
    } else {
      assert(!isReified());
      if (holdsTrue()) {
        // The constraint holds iff any unfixed var takes value false
        invariantGraph().addInvariantNode(std::make_shared<ArrayBoolAndNode>(
            invariantGraph(), std::vector<VarNodeId>{staticInputVarNodeIds()},
            false));
      } else {
        // The constraint holds iff any unfixed var takes value true
        invariantGraph().addInvariantNode(std::make_shared<ArrayBoolOrNode>(
            invariantGraph(), std::vector<VarNodeId>{staticInputVarNodeIds()},
            true));
      }
    }
    return true;
  }
  assert(!isReified());
  if (shouldHold()) {
    const VarNodeId frontVarId = staticInputVarNodeIds().front();
    for (size_t i = 1; i < staticInputVarNodeIds().size(); ++i) {
      invariantGraph().replaceVarNode(staticInputVarNodeIds().at(i),
                                      frontVarId);
    }
    return true;
  }
  assert(staticInputVarNodeIds().size() == 2);
  assert(!isReified() && !shouldHold());
  invariantGraph().addInvariantNode(std::make_shared<ArrayBoolXorNode>(
      invariantGraph(), std::vector<VarNodeId>{staticInputVarNodeIds()}, true));
  return true;
}

void BoolAllEqualNode::registerOutputVars() {
  if (violationVarId() == propagation::NULL_ID) {
    if (shouldHold() || staticInputVarNodeIds().size() == 2) {
      registerViolation();
      assert(_intermediate == propagation::NULL_ID);
    } else if (!shouldHold()) {
      assert(!isReified());
      _intermediate = solver().makeIntVar(0, 0, 0);
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

void BoolAllEqualNode::registerNode() {
  if (staticInputVarNodeIds().empty()) {
    return;
  }
  assert(violationVarId() != propagation::NULL_ID);

  std::vector<propagation::VarViewId> solverVars;
  solverVars.reserve(staticInputVarNodeIds().size());
  std::ranges::transform(
      staticInputVarNodeIds(), std::back_inserter(solverVars),
      [&](const auto& id) { return invariantGraph().varId(id); });

  if (solverVars.size() == 2) {
    assert(_intermediate == propagation::NULL_ID);
    assert(violationVarId().isVar());
    if (shouldHold()) {
      solver().makeViolationInvariant<propagation::BoolEqual>(
          solver(), violationVarId(), solverVars.front(), solverVars.back());

    } else {
      solver().makeInvariant<propagation::BoolXor>(
          solver(), violationVarId(), solverVars.front(), solverVars.back());
    }
    return;
  }

  assert(shouldHold() != (_intermediate != propagation::NULL_ID));
  assert(shouldHold() ? violationVarId().isVar() : violationVarId().isView());

  solver().makeViolationInvariant<propagation::BoolAllEqual>(
      solver(),
      _intermediate == propagation::NULL_ID ? violationVarId() : _intermediate,
      std::move(solverVars));
}

std::string BoolAllEqualNode::dotLangIdentifier() const {
  return "bool_all_equal";
}

}  // namespace atlantis::invariantgraph
