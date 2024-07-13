#include "atlantis/invariantgraph/violationInvariantNodes/boolLeNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/views/boolNotNode.hpp"
#include "atlantis/propagation/violationInvariants/boolLessEqual.hpp"
#include "atlantis/propagation/violationInvariants/boolLessThan.hpp"

namespace atlantis::invariantgraph {

BoolLeNode::BoolLeNode(VarNodeId a, VarNodeId b, VarNodeId r)
    : ViolationInvariantNode(std::vector<VarNodeId>{a, b}, r) {}

BoolLeNode::BoolLeNode(VarNodeId a, VarNodeId b, bool shouldHold)
    : ViolationInvariantNode(std::vector<VarNodeId>{a, b}, shouldHold) {}

void BoolLeNode::init(InvariantGraph& graph, const InvariantNodeId& id) {
  ViolationInvariantNode::init(graph, id);
  assert(!isReified() ||
         !graph.varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(std::none_of(staticInputVarNodeIds().begin(),
                      staticInputVarNodeIds().end(), [&](const VarNodeId& vId) {
                        return graph.varNodeConst(vId).isIntVar();
                      }));
}

void BoolLeNode::updateState(InvariantGraph& graph) {
  ViolationInvariantNode::updateState(graph);
  if (staticInputVarNodeIds().size() < 2) {
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  VarNode& aNode = graph.varNode(a());
  VarNode& bNode = graph.varNode(b());
  if (a() == b()) {
    if (isReified()) {
      graph.varNode(reifiedViolationNodeId()).fixToValue(bool{true});
      ViolationInvariantNode::updateState(graph);
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
      graph.varNode(reifiedViolationNodeId()).fixToValue(!isViolated);
      ViolationInvariantNode::updateState(graph);
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
        graph.varNode(reifiedViolationNodeId()).fixToValue(isSatisfied);
        ViolationInvariantNode::updateState(graph);
      } else if (!shouldHold()) {
        throw InconsistencyException("BoolLeNode neg: a <= b");
      }
      setState(InvariantNodeState::SUBSUMED);
    }
  }
}

bool BoolLeNode::replace(InvariantGraph& graph) {
  if (!canBeReplaced(graph)) {
    return false;
  }
  assert(isReified());
  if (graph.varNode(a()).isFixed()) {
    assert(graph.varNode(a()).inDomain(bool{true}));
    graph.replaceVarNode(reifiedViolationNodeId(), b());
  } else {
    assert(graph.varNode(b()).isFixed() &&
           graph.varNode(b()).inDomain(bool{false}));
    graph.addInvariantNode(
        std::make_unique<BoolNotNode>(a(), reifiedViolationNodeId()));
  }
  return true;
}

bool BoolLeNode::canBeReplaced(const InvariantGraph& graph) const {
  return state() == InvariantNodeState::ACTIVE &&
         staticInputVarNodeIds().size() == 2 &&
         (isReified() && graph.varNodeConst(a()).isFixed() !=
                             graph.varNodeConst(b()).isFixed());
}

void BoolLeNode::registerOutputVars(InvariantGraph& invariantGraph,
                                    propagation::SolverBase& solver) {
  registerViolation(invariantGraph, solver);
}

void BoolLeNode::registerNode(InvariantGraph& invariantGraph,
                              propagation::SolverBase& solver) {
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);
  assert(invariantGraph.varId(a()) != propagation::NULL_ID);
  assert(invariantGraph.varId(b()) != propagation::NULL_ID);

  if (shouldHold()) {
    solver.makeViolationInvariant<propagation::BoolLessEqual>(
        solver, violationVarId(invariantGraph), invariantGraph.varId(a()),
        invariantGraph.varId(b()));
  } else {
    assert(!isReified());
    solver.makeViolationInvariant<propagation::BoolLessThan>(
        solver, violationVarId(invariantGraph), invariantGraph.varId(b()),
        invariantGraph.varId(a()));
  }
}

}  // namespace atlantis::invariantgraph
