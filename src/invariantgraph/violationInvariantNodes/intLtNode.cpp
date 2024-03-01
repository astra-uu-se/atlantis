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

void IntLtNode::registerOutputVars(InvariantGraph& invariantGraph,
                                   propagation::SolverBase& solver) {
  registerViolation(invariantGraph, solver);
}

void IntLtNode::registerNode(InvariantGraph& invariantGraph,
                             propagation::SolverBase& solver) {
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);

  if (shouldHold()) {
    solver.makeViolationInvariant<propagation::LessThan>(
        solver, violationVarId(invariantGraph), invariantGraph.varId(a()),
        invariantGraph.varId(b()));
  } else {
    assert(!isReified());
    solver.makeViolationInvariant<propagation::LessEqual>(
        solver, violationVarId(invariantGraph), invariantGraph.varId(b()),
        invariantGraph.varId(a()));
  }
}

}  // namespace atlantis::invariantgraph
