#include "atlantis/invariantgraph/violationInvariantNodes/boolLeNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/views/boolNotNode.hpp"
#include "atlantis/propagation/violationInvariants/boolLessEqual.hpp"
#include "atlantis/propagation/violationInvariants/boolLessThan.hpp"

namespace atlantis::invariantgraph {

BoolLeNode::BoolLeNode(IInvariantGraph& graph, VarNodeId a, VarNodeId b,
                       VarNodeId r)
    : ViolationInvariantNode(graph, std::vector<VarNodeId>{a, b}, r) {}

BoolLeNode::BoolLeNode(IInvariantGraph& graph, VarNodeId a, VarNodeId b,
                       bool shouldHold)
    : ViolationInvariantNode(graph, std::vector<VarNodeId>{a, b}, shouldHold) {}

void BoolLeNode::init(InvariantNodeId id) {
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

void BoolLeNode::updateState() {
  ViolationInvariantNode::updateState();
  if (staticInputVarNodeIds().size() < 2) {
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  VarNode& aNode = invariantGraph().varNode(a());
  VarNode& bNode = invariantGraph().varNode(b());
  if (a() == b()) {
    if (isReified()) {
      fixReified(true);
    } else if (!shouldHold()) {
      throw InconsistencyException("BoolLeNode neg: a == b");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (!isReified() && !shouldHold()) {
    aNode.fixToValue(bool{true});
    bNode.fixToValue(bool{false});
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (aNode.isFixed() && bNode.isFixed()) {
    const bool isViolated =
        aNode.inDomain(bool{true}) && bNode.inDomain(bool{false});
    if (isReified()) {
      fixReified(!isViolated);
    } else if (isViolated == shouldHold()) {
      throw InconsistencyException(shouldHold() ? "BoolLeNode: a > b"
                                                : "BoolLeNode neg: a <= b");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  } else if (aNode.isFixed() || bNode.isFixed()) {
    assert(aNode.isFixed() != bNode.isFixed());
    const bool isSatisfied = (aNode.isFixed() && aNode.inDomain(bool{false})) ||
                             (bNode.isFixed() && bNode.inDomain(bool{true}));
    if (isSatisfied) {
      if (isReified()) {
        fixReified(isSatisfied);
      } else if (!shouldHold()) {
        throw InconsistencyException("BoolLeNode neg: a <= b");
      }
      setState(InvariantNodeState::SUBSUMED);
    }
  }
}

bool BoolLeNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  assert(isReified());
  if (invariantGraph().varNode(a()).isFixed()) {
    assert(invariantGraph().varNode(a()).inDomain(bool{true}));
    invariantGraph().replaceVarNode(reifiedViolationNodeId(), b());
  } else {
    assert(invariantGraph().varNode(b()).isFixed() &&
           invariantGraph().varNode(b()).inDomain(bool{false}));
    invariantGraph().addInvariantNode(std::make_shared<BoolNotNode>(
        invariantGraph(), a(), reifiedViolationNodeId()));
  }
  return true;
}

bool BoolLeNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         staticInputVarNodeIds().size() == 2 &&
         (isReified() && invariantGraphConst().varNodeConst(a()).isFixed() !=
                             invariantGraphConst().varNodeConst(b()).isFixed());
}

void BoolLeNode::registerOutputVars() {
  registerViolation();
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId vId) {
                       return invariantGraphConst().varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void BoolLeNode::registerNode() {
  assert(violationVarId() != propagation::NULL_ID);
  assert(invariantGraph().varId(a()) != propagation::NULL_ID);
  assert(invariantGraph().varId(b()) != propagation::NULL_ID);
  assert(violationVarId().isVar());

  if (shouldHold()) {
    solver().makeViolationInvariant<propagation::BoolLessEqual>(
        solver(), violationVarId(), invariantGraph().varId(a()),
        invariantGraph().varId(b()));
  } else {
    assert(!isReified());
    solver().makeViolationInvariant<propagation::BoolLessThan>(
        solver(), violationVarId(), invariantGraph().varId(b()),
        invariantGraph().varId(a()));
  }
}

std::string BoolLeNode::dotLangIdentifier() const { return "bool_le"; }

}  // namespace atlantis::invariantgraph
