#include "atlantis/invariantgraph/violationInvariantNodes/boolLtNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/views/boolNotNode.hpp"
#include "atlantis/propagation/violationInvariants/boolLessEqual.hpp"
#include "atlantis/propagation/violationInvariants/boolLessThan.hpp"

namespace atlantis::invariantgraph {

BoolLtNode::BoolLtNode(VarNodeId a, VarNodeId b, VarNodeId r)
    : ViolationInvariantNode(std::vector<VarNodeId>{a, b}, r) {}
BoolLtNode::BoolLtNode(VarNodeId a, VarNodeId b, bool shouldHold)
    : ViolationInvariantNode(std::vector<VarNodeId>{a, b}, shouldHold) {}

void BoolLtNode::updateState(InvariantGraph& graph) {
  ViolationInvariantNode::updateState(graph);
  if (staticInputVarNodeIds().size() < 2) {
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  VarNode& aNode = graph.varNode(a());
  VarNode& bNode = graph.varNode(b());
  if (a() == b()) {
    if (isReified()) {
      graph.varNode(reifiedViolationNodeId()).fixToValue(bool{false});
      ViolationInvariantNode::updateState(graph);
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
      graph.varNode(reifiedViolationNodeId()).fixToValue(isSatisifed);
      ViolationInvariantNode::updateState(graph);
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
        graph.varNode(reifiedViolationNodeId()).fixToValue(!isViolated);
        ViolationInvariantNode::updateState(graph);
      } else if (shouldHold()) {
        throw InconsistencyException("BoolLtNode: a >= b");
      }
      setState(InvariantNodeState::SUBSUMED);
    }
  }
}

bool BoolLtNode::canBeReplaced(const InvariantGraph& graph) const {
  return state() == InvariantNodeState::ACTIVE &&
         staticInputVarNodeIds().size() == 2 &&
         (isReified() && graph.varNodeConst(a()).isFixed() !=
                             graph.varNodeConst(b()).isFixed());
}

bool BoolLtNode::replace(InvariantGraph& graph) {
  if (!canBeReplaced(graph)) {
    return false;
  }
  assert(isReified());
  if (graph.varNode(a()).isFixed()) {
    assert(graph.varNode(a()).inDomain(bool{false}));
    graph.replaceVarNode(reifiedViolationNodeId(), b());
  } else {
    assert(graph.varNode(b()).isFixed() &&
           graph.varNode(b()).inDomain(bool{true}));
    graph.addInvariantNode(
        std::make_unique<BoolNotNode>(a(), reifiedViolationNodeId()));
  }
  return true;
}

void BoolLtNode::registerOutputVars(InvariantGraph& invariantGraph,
                                    propagation::SolverBase& solver) {
  registerViolation(invariantGraph, solver);
}

void BoolLtNode::registerNode(InvariantGraph& invariantGraph,
                              propagation::SolverBase& solver) {
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);
  assert(invariantGraph.varId(a()) != propagation::NULL_ID);
  assert(invariantGraph.varId(b()) != propagation::NULL_ID);

  if (shouldHold()) {
    solver.makeViolationInvariant<propagation::BoolLessThan>(
        solver, violationVarId(invariantGraph), invariantGraph.varId(a()),
        invariantGraph.varId(b()));
  } else {
    assert(!isReified());
    solver.makeViolationInvariant<propagation::BoolLessEqual>(
        solver, violationVarId(invariantGraph), invariantGraph.varId(b()),
        invariantGraph.varId(a()));
  }
}

}  // namespace atlantis::invariantgraph
