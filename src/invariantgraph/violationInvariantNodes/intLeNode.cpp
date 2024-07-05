#include "atlantis/invariantgraph/violationInvariantNodes/intLeNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/violationInvariants/lessEqual.hpp"
#include "atlantis/propagation/violationInvariants/lessThan.hpp"

namespace atlantis::invariantgraph {

IntLeNode::IntLeNode(VarNodeId a, VarNodeId b, VarNodeId r)
    : ViolationInvariantNode({a, b}, r) {}

IntLeNode::IntLeNode(VarNodeId a, VarNodeId b, bool shouldHold)
    : ViolationInvariantNode({a, b}, shouldHold) {}

void IntLeNode::registerOutputVars(InvariantGraph& invariantGraph,
                                   propagation::SolverBase& solver) {
  registerViolation(invariantGraph, solver);
}

void IntLeNode::updateState(InvariantGraph& graph) {
  ViolationInvariantNode::updateState(graph);
  if (staticInputVarNodeIds().size() < 2) {
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  VarNode aNode = graph.varNode(a());
  VarNode bNode = graph.varNode(b());
  if (a() == b()) {
    if (isReified()) {
      graph.varNode(reifiedViolationNodeId()).fixToValue(bool{true});
      ViolationInvariantNode::updateState(graph);
    } else if (!shouldHold()) {
      throw InconsistencyException("IntLeNode neg: a == b");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (!isReified()) {
    if (shouldHold()) {
      // a <= b
      aNode.removeValuesAbove(bNode.upperBound());
      bNode.removeValuesBelow(aNode.lowerBound());
    } else {
      // a > b
      aNode.removeValuesBelow(bNode.lowerBound() + 1);
      bNode.removeValuesAbove(aNode.upperBound() - 1);
    }
  }
  if (aNode.upperBound() <= bNode.lowerBound()) {
    // always true
    if (isReified()) {
      graph.varNode(reifiedViolationNodeId()).fixToValue(bool{true});
      ViolationInvariantNode::updateState(graph);
    } else if (!shouldHold()) {
      throw InconsistencyException("IntLeNode neg: a <= b");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  } else if (aNode.lowerBound() > bNode.upperBound()) {
    // always false
    if (isReified()) {
      graph.varNode(reifiedViolationNodeId()).fixToValue(bool{false});
      ViolationInvariantNode::updateState(graph);
    } else if (shouldHold()) {
      throw InconsistencyException("IntLeNode: a > b");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
}

void IntLeNode::registerNode(InvariantGraph& invariantGraph,
                             propagation::SolverBase& solver) {
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);

  if (shouldHold()) {
    solver.makeViolationInvariant<propagation::LessEqual>(
        solver, violationVarId(invariantGraph), invariantGraph.varId(a()),
        invariantGraph.varId(b()));
  } else {
    assert(!isReified());
    solver.makeViolationInvariant<propagation::LessThan>(
        solver, violationVarId(invariantGraph), invariantGraph.varId(b()),
        invariantGraph.varId(a()));
  }
}

}  // namespace atlantis::invariantgraph
