#include "atlantis/invariantgraph/violationInvariantNodes/boolXorNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/boolXor.hpp"
#include "atlantis/propagation/violationInvariants/boolEqual.hpp"

namespace atlantis::invariantgraph {

BoolXorNode::BoolXorNode(VarNodeId a, VarNodeId b, VarNodeId r)
    : ViolationInvariantNode({a, b}, r) {}
BoolXorNode::BoolXorNode(VarNodeId a, VarNodeId b, bool shouldHold)
    : ViolationInvariantNode({a, b}, shouldHold) {}

void BoolXorNode::registerOutputVars(InvariantGraph& invariantGraph,
                                     propagation::SolverBase& solver) {
  registerViolation(invariantGraph, solver);
}

void BoolXorNode::registerNode(InvariantGraph& invariantGraph,
                               propagation::SolverBase& solver) {
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);

  if (shouldHold()) {
    solver.makeInvariant<propagation::BoolXor>(
        solver, violationVarId(invariantGraph), invariantGraph.varId(a()),
        invariantGraph.varId(b()));
  } else {
    assert(!isReified());
    solver.makeViolationInvariant<propagation::BoolEqual>(
        solver, violationVarId(invariantGraph), invariantGraph.varId(a()),
        invariantGraph.varId(b()));
  }
}

}  // namespace atlantis::invariantgraph
