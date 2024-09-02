#include "atlantis/invariantgraph/violationInvariantNodes/intLtNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/violationInvariants/lessEqual.hpp"
#include "atlantis/propagation/violationInvariants/lessThan.hpp"

namespace atlantis::invariantgraph {

IntLtNode::IntLtNode(IInvariantGraph& graph, VarNodeId a, VarNodeId b,
                     VarNodeId r)
    : ViolationInvariantNode(graph, {a, b}, r) {}

IntLtNode::IntLtNode(IInvariantGraph& graph, VarNodeId a, VarNodeId b,
                     bool shouldHold)
    : ViolationInvariantNode(graph, {a, b}, shouldHold) {}

void IntLtNode::init(InvariantNodeId id) {
  ViolationInvariantNode::init(id);
  assert(
      !isReified() ||
      !invariantGraphConst().varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(
      std::all_of(staticInputVarNodeIds().begin(),
                  staticInputVarNodeIds().end(), [&](const VarNodeId vId) {
                    return invariantGraphConst().varNodeConst(vId).isIntVar();
                  }));
}

void IntLtNode::updateState() {
  ViolationInvariantNode::updateState();
  if (staticInputVarNodeIds().size() < 2) {
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  VarNode aNode = invariantGraph().varNode(a());
  VarNode bNode = invariantGraph().varNode(b());
  if (a() == b()) {
    if (isReified()) {
      fixReified(false);
    } else if (shouldHold()) {
      throw InconsistencyException("IntLtNode: a == b");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (!isReified()) {
    if (shouldHold()) {
      // a < b
      // aNode.removeValuesAbove(bNode.upperBound() - 1);
      // bNode.removeValuesBelow(aNode.lowerBound() + 1);
    } else {
      // a >= b
      // aNode.removeValuesBelow(bNode.lowerBound());
      // bNode.removeValuesAbove(aNode.upperBound());
    }
  }
  if (aNode.upperBound() < bNode.lowerBound()) {
    // always true
    if (isReified()) {
      fixReified(true);
    } else if (!shouldHold()) {
      throw InconsistencyException("IntLtNode neg: a < b");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  } else if (aNode.lowerBound() >= bNode.upperBound()) {
    // always false
    if (isReified()) {
      fixReified(false);
    } else if (shouldHold()) {
      throw InconsistencyException("IntLtNode: a >= b");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
}

void IntLtNode::registerOutputVars() {
  registerViolation();
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId vId) {
                       return invariantGraphConst().varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void IntLtNode::registerNode() {
  assert(violationVarId() != propagation::NULL_ID);
  assert(violationVarId().isVar());

  if (shouldHold()) {
    solver().makeViolationInvariant<propagation::LessThan>(
        solver(), violationVarId(), invariantGraph().varId(a()),
        invariantGraph().varId(b()));
  } else {
    assert(!isReified());
    solver().makeViolationInvariant<propagation::LessEqual>(
        solver(), violationVarId(), invariantGraph().varId(b()),
        invariantGraph().varId(a()));
  }
}

std::string IntLtNode::dotLangIdentifier() const { return "int_lt_node"; }

}  // namespace atlantis::invariantgraph
