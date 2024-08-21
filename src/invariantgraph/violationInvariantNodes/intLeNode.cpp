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

void IntLeNode::init(InvariantGraph& graph, const InvariantNodeId& id) {
  ViolationInvariantNode::init(graph, id);
  assert(!isReified() ||
         !graph.varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(std::all_of(staticInputVarNodeIds().begin(),
                     staticInputVarNodeIds().end(), [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).isIntVar();
                     }));
}

void IntLeNode::registerOutputVars(InvariantGraph& graph,
                                   propagation::SolverBase& solver) {
  registerViolation(graph, solver);
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
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
      fixReified(graph, true);
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
      fixReified(graph, true);
    } else if (!shouldHold()) {
      throw InconsistencyException("IntLeNode neg: a <= b");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  } else if (aNode.lowerBound() > bNode.upperBound()) {
    // always false
    if (isReified()) {
      fixReified(graph, false);
    } else if (shouldHold()) {
      throw InconsistencyException("IntLeNode: a > b");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
}

void IntLeNode::registerNode(InvariantGraph& graph,
                             propagation::SolverBase& solver) {
  assert(violationVarId(graph) != propagation::NULL_ID);

  if (shouldHold()) {
    solver.makeViolationInvariant<propagation::LessEqual>(
        solver, violationVarId(graph), graph.varId(a()), graph.varId(b()));
  } else {
    assert(!isReified());
    solver.makeViolationInvariant<propagation::LessThan>(
        solver, violationVarId(graph), graph.varId(b()), graph.varId(a()));
  }
}

}  // namespace atlantis::invariantgraph
