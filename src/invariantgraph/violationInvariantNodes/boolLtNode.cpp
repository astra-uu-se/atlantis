#include "invariantgraph/violationInvariantNodes/boolLtNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

BoolLtNode::BoolLtNode(VarNodeId a, VarNodeId b, VarNodeId r)
    : ViolationInvariantNode(std::vector<VarNodeId>{a, b}, r) {}
BoolLtNode::BoolLtNode(VarNodeId a, VarNodeId b, bool shouldHold)
    : ViolationInvariantNode(std::vector<VarNodeId>{a, b},
                             shouldHold) {}

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