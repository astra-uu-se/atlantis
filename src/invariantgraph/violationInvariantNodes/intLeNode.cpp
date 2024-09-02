#include "atlantis/invariantgraph/violationInvariantNodes/intLeNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/violationInvariants/lessEqual.hpp"
#include "atlantis/propagation/violationInvariants/lessThan.hpp"

namespace atlantis::invariantgraph {

IntLeNode::IntLeNode(IInvariantGraph& graph, VarNodeId a, VarNodeId b,
                     VarNodeId r)
    : ViolationInvariantNode(graph, {a, b}, r) {}

IntLeNode::IntLeNode(IInvariantGraph& graph, VarNodeId a, VarNodeId b,
                     bool shouldHold)
    : ViolationInvariantNode(graph, {a, b}, shouldHold) {}

void IntLeNode::init(InvariantNodeId id) {
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

void IntLeNode::registerOutputVars() {
  registerViolation();
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId vId) {
                       return invariantGraphConst().varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void IntLeNode::updateState() {
  ViolationInvariantNode::updateState();
  if (staticInputVarNodeIds().size() < 2) {
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  VarNode aNode = invariantGraph().varNode(a());
  VarNode bNode = invariantGraph().varNode(b());
  if (a() == b()) {
    if (isReified()) {
      fixReified(true);
    } else if (!shouldHold()) {
      throw InconsistencyException("IntLeNode neg: a == b");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  /*
  if (!isReified()) {
    if (shouldHold()) {
      a <= b
      aNode.removeValuesAbove(bNode.upperBound());
      bNode.removeValuesBelow(aNode.lowerBound());
    } else {
      a > b
      aNode.removeValuesBelow(bNode.lowerBound() + 1);
      bNode.removeValuesAbove(aNode.upperBound() - 1);
    }
  }
  */
  if (aNode.upperBound() <= bNode.lowerBound()) {
    // always true
    if (isReified()) {
      fixReified(true);
    } else if (!shouldHold()) {
      throw InconsistencyException("IntLeNode neg: a <= b");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  } else if (aNode.lowerBound() > bNode.upperBound()) {
    // always false
    if (isReified()) {
      fixReified(false);
    } else if (shouldHold()) {
      throw InconsistencyException("IntLeNode: a > b");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
}

void IntLeNode::registerNode() {
  assert(violationVarId() != propagation::NULL_ID);
  assert(violationVarId().isVar());

  if (shouldHold()) {
    solver().makeViolationInvariant<propagation::LessEqual>(
        solver(), violationVarId(), invariantGraph().varId(a()),
        invariantGraph().varId(b()));
  } else {
    assert(!isReified());
    solver().makeViolationInvariant<propagation::LessThan>(
        solver(), violationVarId(), invariantGraph().varId(b()),
        invariantGraph().varId(a()));
  }
}

std::string IntLeNode::dotLangIdentifier() const { return "int_le_node"; }

}  // namespace atlantis::invariantgraph
