#include "atlantis/invariantgraph/invariantNodes/intDivNode.hpp"

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/intDiv.hpp"

namespace atlantis::invariantgraph {

IntDivNode::IntDivNode(VarNodeId numerator, VarNodeId denominator,
                       VarNodeId quotient)
    : InvariantNode({quotient}, {numerator, denominator}) {}

void IntDivNode::propagate(InvariantGraph& graph) {
  if (graph.isFixed(denominator()) && graph.lowerBound(denominator()) == 0) {
    setState(InvariantNodeState::INFEASIBLE);
    return;
  }
  if (graph.isFixed(numerator()) && graph.isFixed(denominator())) {
    if (graph.lowerBound(numerator()) % graph.lowerBound(denominator()) != 0) {
      setState(InvariantNodeState::INFEASIBLE);
      return;
    }
    if (graph.isFixed(quotient())) {
      if (graph.lowerBound(numerator()) / graph.lowerBound(denominator()) !=
          graph.lowerBound(quotient())) {
        setState(InvariantNodeState::INFEASIBLE);
        return;
      }
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
  }

  if (graph.isFixed(quotient()) && graph.lowerBound(quotient()) == 0) {
    graph.fixToValue(numerator(), 0);
    setState(InvariantNodeState::SUBSUMED);
    return;
  }

  if (graph.isFixed(numerator()) && graph.isFixed(quotient())) {
    const Int numVal = graph.lowerBound(numerator());
    const Int quoVal = graph.lowerBound(quotient());
    if (numVal % quoVal != 0) {
      setState(InvariantNodeState::INFEASIBLE);
      return;
    }

    assert(quoVal != 0);
    const int denVal = numVal / quoVal;
    graph.fixToValue(denominator(), denVal);
    setState(InvariantNodeState::SUBSUMED);
    return;
  } else if (graph.isFixed(denominator()) && graph.isFixed(quotient())) {
    const Int denVal = graph.lowerBound(denominator());
    const Int quoVal = graph.lowerBound(quotient());
    const Int numVal = denVal * quoVal;
    graph.fixToValue(numerator(), numVal);
    setState(InvariantNodeState::SUBSUMED);
  }

  if (graph.isFixed(denominator()) && graph.lowerBound(denominator()) == 1) {
    setState(InvariantNodeState::REPLACABLE);
  }

  return;
}

bool IntDivNode::replace(InvariantGraph& invariantGraph) {
  if (state() != InvariantNodeState::REPLACABLE) {
    return false;
  }
  assert(invariantGraph.varNode(denominator()).isFixed() &&
         invariantGraph.varNode(denominator()).lowerBound() == 1);
  invariantGraph.replaceVarNode(quotient(), numerator());
  return true;
}

void invariantgraph::IntDivNode::registerOutputVars(
    InvariantGraph& invariantGraph, propagation::SolverBase& solver) {
  makeSolverVar(solver, invariantGraph.varNode(quotient()));
}

void invariantgraph::IntDivNode::registerNode(InvariantGraph& invariantGraph,
                                              propagation::SolverBase& solver) {
  assert(invariantGraph.varId(quotient()) != propagation::NULL_ID);
  solver.makeInvariant<propagation::IntDiv>(
      solver, invariantGraph.varId(quotient()),
      invariantGraph.varId(numerator()), invariantGraph.varId(denominator()));
}

VarNodeId IntDivNode::numerator() const noexcept {
  return staticInputVarNodeIds().front();
}
VarNodeId IntDivNode::denominator() const noexcept {
  return staticInputVarNodeIds().back();
}
VarNodeId IntDivNode::quotient() const noexcept {
  return outputVarNodeIds().front();
}

}  // namespace atlantis::invariantgraph
