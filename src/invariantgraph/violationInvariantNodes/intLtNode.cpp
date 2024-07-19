#include "atlantis/invariantgraph/violationInvariantNodes/intLtNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/violationInvariants/lessEqual.hpp"
#include "atlantis/propagation/violationInvariants/lessThan.hpp"

namespace atlantis::invariantgraph {

IntLtNode::IntLtNode(VarNodeId a, VarNodeId b, VarNodeId r)
    : ViolationInvariantNode({a, b}, r) {}

IntLtNode::IntLtNode(VarNodeId a, VarNodeId b, bool shouldHold)
    : ViolationInvariantNode({a, b}, shouldHold) {}

void IntLtNode::init(InvariantGraph& graph, const InvariantNodeId& id) {
  ViolationInvariantNode::init(graph, id);
  assert(!isReified() ||
         !graph.varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(std::all_of(staticInputVarNodeIds().begin(),
                     staticInputVarNodeIds().end(), [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).isIntVar();
                     }));
}

void IntLtNode::updateState(InvariantGraph& graph) {
  ViolationInvariantNode::updateState(graph);
  if (staticInputVarNodeIds().size() < 2) {
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  VarNode aNode = graph.varNode(a());
  VarNode bNode = graph.varNode(b());
  if (a() == b()) {
    if (isReified()) {
      graph.varNode(reifiedViolationNodeId()).fixToValue(bool{false});
      ViolationInvariantNode::updateState(graph);
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
      graph.varNode(reifiedViolationNodeId()).fixToValue(bool{true});
      ViolationInvariantNode::updateState(graph);
    } else if (!shouldHold()) {
      throw InconsistencyException("IntLtNode neg: a < b");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  } else if (aNode.lowerBound() >= bNode.upperBound()) {
    // always false
    if (isReified()) {
      graph.varNode(reifiedViolationNodeId()).fixToValue(bool{false});
      ViolationInvariantNode::updateState(graph);
    } else if (shouldHold()) {
      throw InconsistencyException("IntLtNode: a >= b");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
}

void IntLtNode::registerOutputVars(InvariantGraph& graph,
                                   propagation::SolverBase& solver) {
  registerViolation(graph, solver);
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void IntLtNode::registerNode(InvariantGraph& graph,
                             propagation::SolverBase& solver) {
  assert(violationVarId(graph) != propagation::NULL_ID);

  if (shouldHold()) {
    solver.makeViolationInvariant<propagation::LessThan>(
        solver, violationVarId(graph), graph.varId(a()), graph.varId(b()));
  } else {
    assert(!isReified());
    solver.makeViolationInvariant<propagation::LessEqual>(
        solver, violationVarId(graph), graph.varId(b()), graph.varId(a()));
  }
}

}  // namespace atlantis::invariantgraph
