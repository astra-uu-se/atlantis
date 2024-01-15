#include "invariantgraph/violationInvariantNodes/boolEqNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

BoolEqNode::BoolEqNode(VarNodeId a, VarNodeId b, VarNodeId r)
    : ViolationInvariantNode(std::move(std::vector<VarNodeId>{a, b}), r) {}

BoolEqNode::BoolEqNode(VarNodeId a, VarNodeId b, bool shouldHold)
    : ViolationInvariantNode(std::move(std::vector<VarNodeId>{a, b}),
                             shouldHold) {}

void BoolEqNode::registerOutputVars(InvariantGraph& invariantGraph,
                                    propagation::SolverBase& solver) {
  registerViolation(invariantGraph, solver);
}

void BoolEqNode::registerNode(InvariantGraph& invariantGraph,
                              propagation::SolverBase& solver) {
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);
  assert(invariantGraph.varId(a()) != propagation::NULL_ID);
  assert(invariantGraph.varId(b()) != propagation::NULL_ID);

  if (shouldHold()) {
    solver.makeViolationInvariant<propagation::BoolEqual>(
        solver, violationVarId(invariantGraph), invariantGraph.varId(a()),
        invariantGraph.varId(b()));
  } else {
    solver.makeInvariant<propagation::BoolXor>(
        solver, violationVarId(invariantGraph), invariantGraph.varId(a()),
        invariantGraph.varId(b()));
  }
}

}  // namespace atlantis::invariantgraph