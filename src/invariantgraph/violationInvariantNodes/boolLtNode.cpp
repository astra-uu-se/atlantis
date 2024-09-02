#include "atlantis/invariantgraph/violationInvariantNodes/boolLtNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/views/boolNotNode.hpp"
#include "atlantis/propagation/violationInvariants/boolLessEqual.hpp"
#include "atlantis/propagation/violationInvariants/boolLessThan.hpp"

namespace atlantis::invariantgraph {

BoolLtNode::BoolLtNode(IInvariantGraph& graph, VarNodeId a, VarNodeId b,
                       VarNodeId r)
    : ViolationInvariantNode(graph, std::vector<VarNodeId>{a, b}, r) {}

BoolLtNode::BoolLtNode(IInvariantGraph& graph, VarNodeId a, VarNodeId b,
                       bool shouldHold)
    : ViolationInvariantNode(graph, std::vector<VarNodeId>{a, b}, shouldHold) {}

void BoolLtNode::init(InvariantNodeId id) {
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

void BoolLtNode::updateState() {
  ViolationInvariantNode::updateState();
  if (staticInputVarNodeIds().size() < 2) {
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  VarNode& aNode = invariantGraph().varNode(a());
  VarNode& bNode = invariantGraph().varNode(b());
  if (a() == b()) {
    if (isReified()) {
      fixReified(false);
    } else if (shouldHold()) {
      throw InconsistencyException("BoolLtNode: a == b");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (!isReified() && shouldHold()) {
    aNode.fixToValue(bool{false});
    bNode.fixToValue(bool{true});
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (aNode.isFixed() && bNode.isFixed()) {
    const bool isSatisifed =
        aNode.inDomain(bool{false}) && bNode.inDomain(bool{true});
    if (isReified()) {
      fixReified(isSatisifed);
    } else if (isSatisifed != shouldHold()) {
      throw InconsistencyException(shouldHold() ? "BoolLtNode: a >= b"
                                                : "BoolLtNode neg: a < b");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  } else if (aNode.isFixed() || bNode.isFixed()) {
    assert(aNode.isFixed() != bNode.isFixed());
    const bool isViolated = (aNode.isFixed() && aNode.inDomain(bool{true})) ||
                            (bNode.isFixed() && bNode.inDomain(bool{false}));
    if (isViolated) {
      if (isReified()) {
        fixReified(!isViolated);
      } else if (shouldHold()) {
        throw InconsistencyException("BoolLtNode: a >= b");
      }
      setState(InvariantNodeState::SUBSUMED);
    }
  }
}

bool BoolLtNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         staticInputVarNodeIds().size() == 2 &&
         (isReified() && invariantGraphConst().varNodeConst(a()).isFixed() !=
                             invariantGraphConst().varNodeConst(b()).isFixed());
}

bool BoolLtNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  assert(isReified());
  if (invariantGraph().varNode(a()).isFixed()) {
    assert(invariantGraph().varNode(a()).inDomain(bool{false}));
    invariantGraph().replaceVarNode(reifiedViolationNodeId(), b());
  } else {
    assert(invariantGraph().varNode(b()).isFixed() &&
           invariantGraph().varNode(b()).inDomain(bool{true}));
    invariantGraph().addInvariantNode(std::make_shared<BoolNotNode>(
        invariantGraph(), a(), reifiedViolationNodeId()));
  }
  return true;
}

void BoolLtNode::registerOutputVars() {
  registerViolation();
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId vId) {
                       return invariantGraphConst().varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void BoolLtNode::registerNode() {
  assert(violationVarId() != propagation::NULL_ID);
  assert(violationVarId().isVar());

  assert(invariantGraph().varId(a()) != propagation::NULL_ID);
  assert(invariantGraph().varId(b()) != propagation::NULL_ID);

  if (shouldHold()) {
    solver().makeViolationInvariant<propagation::BoolLessThan>(
        solver(), violationVarId(), invariantGraph().varId(a()),
        invariantGraph().varId(b()));
  } else {
    assert(!isReified());
    solver().makeViolationInvariant<propagation::BoolLessEqual>(
        solver(), violationVarId(), invariantGraph().varId(b()),
        invariantGraph().varId(a()));
  }
}

std::string BoolLtNode::dotLangIdentifier() const { return "bool_lt"; }

}  // namespace atlantis::invariantgraph
