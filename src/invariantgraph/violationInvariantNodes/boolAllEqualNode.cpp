#include "atlantis/invariantgraph/violationInvariantNodes/boolAllEqualNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolXorNode.hpp"
#include "atlantis/propagation/invariants/boolXor.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/propagation/violationInvariants/boolAllEqual.hpp"
#include "atlantis/propagation/violationInvariants/boolEqual.hpp"

namespace atlantis::invariantgraph {

BoolAllEqualNode::BoolAllEqualNode(IInvariantGraph& graph, VarNodeId a,
                                   VarNodeId b, VarNodeId r, bool breaksCycle)
    : BoolAllEqualNode(graph, std::vector<VarNodeId>{a, b}, r, breaksCycle) {}

BoolAllEqualNode::BoolAllEqualNode(IInvariantGraph& graph, VarNodeId a,
                                   VarNodeId b, bool shouldHold,
                                   bool breaksCycle)
    : BoolAllEqualNode(graph, std::vector<VarNodeId>{a, b}, shouldHold,
                       breaksCycle) {}

BoolAllEqualNode::BoolAllEqualNode(IInvariantGraph& graph,
                                   std::vector<VarNodeId>&& vars, VarNodeId r,
                                   bool breaksCycle)
    : ViolationInvariantNode(graph, std::move(vars), r),
      _breaksCycle(breaksCycle) {}

BoolAllEqualNode::BoolAllEqualNode(IInvariantGraph& graph,
                                   std::vector<VarNodeId>&& vars,
                                   bool shouldHold, bool breaksCycle)
    : ViolationInvariantNode(graph, std::move(vars), shouldHold),
      _breaksCycle(breaksCycle) {}

void BoolAllEqualNode::init(InvariantNodeId id) {
  ViolationInvariantNode::init(id);
  assert(
      !isReified() ||
      !invariantGraphConst().varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(
      std::none_of(staticInputVarNodeIds().begin(),
                   staticInputVarNodeIds().end(), [&](const VarNodeId vId) {
                     return invariantGraphConst().varNodeConst(vId).isIntVar();
                   }));
}

void BoolAllEqualNode::updateState() {
  ViolationInvariantNode::updateState();
  if (staticInputVarNodeIds().size() < 2) {
    if (isReified()) {
      fixReified(true);
    } else if (!shouldHold()) {
      throw InconsistencyException(
          "BoolAllEqualNode::updateState constraint is violated");
    }
    if (isReified()) {
      fixReified(true);
    }
    setState(InvariantNodeState::SUBSUMED);
  }
  size_t numFixed = 0;
  for (size_t i = 0; i < staticInputVarNodeIds().size(); ++i) {
    const VarNode& iNode =
        invariantGraphConst().varNodeConst(staticInputVarNodeIds().at(i));
    if (!iNode.isFixed()) {
      continue;
    }
    ++numFixed;
    const bool iVal = iNode.inDomain(bool{true});
    for (size_t j = i + 1; j < staticInputVarNodeIds().size(); ++j) {
      const VarNode& jNode =
          invariantGraphConst().varNodeConst(staticInputVarNodeIds().at(j));
      if (!jNode.isFixed()) {
        continue;
      }
      const bool jVal = jNode.inDomain(bool{true});
      if (iVal != jVal) {
        if (isReified()) {
          fixReified(false);
        } else if (shouldHold()) {
          throw InconsistencyException(
              "BoolAllEqualNode::updateState constraint is violated");
        }
      }
    }
  }
  if (numFixed == staticInputVarNodeIds().size()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool BoolAllEqualNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE && !_breaksCycle &&
         (!isReified() &&
          (shouldHold() || staticInputVarNodeIds().size() <= 2));
}

bool BoolAllEqualNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (!shouldHold()) {
    assert(staticInputVarNodeIds().size() == 2);
    invariantGraph().addInvariantNode(std::make_shared<ArrayBoolXorNode>(
        invariantGraph(), std::vector<VarNodeId>{staticInputVarNodeIds()},
        true));
  } else if (!staticInputVarNodeIds().empty()) {
    const VarNodeId firstVar = staticInputVarNodeIds().front();
    for (size_t i = 1; i < staticInputVarNodeIds().size(); ++i) {
      invariantGraph().replaceVarNode(staticInputVarNodeIds().at(i), firstVar);
    }
  }
  return true;
}

void BoolAllEqualNode::registerOutputVars() {
  if (violationVarId() == propagation::NULL_ID) {
    if (shouldHold() || staticInputVarNodeIds().size() == 2) {
      registerViolation();
    } else if (!shouldHold()) {
      assert(!isReified());
      _intermediate = solver().makeIntVar(0, 0, 0);
      setViolationVarId(solver().makeIntView<propagation::NotEqualConst>(
          solver(), _intermediate, 0));
    }
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
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
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars),
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

  assert(shouldHold() || _intermediate != propagation::NULL_ID);
  assert(shouldHold() ? violationVarId().isVar() : _intermediate.isVar());

  solver().makeViolationInvariant<propagation::BoolAllEqual>(
      solver(), !shouldHold() ? _intermediate : violationVarId(),
      std::move(solverVars));
}

std::string BoolAllEqualNode::dotLangIdentifier() const {
  return "bool_all_equal";
}

}  // namespace atlantis::invariantgraph
