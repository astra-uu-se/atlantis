#include "invariantgraph/violationInvariantNodes/boolAndNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

BoolAndNode::BoolAndNode(VarNodeId a, VarNodeId b, VarNodeId r)
    : ViolationInvariantNode(std::vector<VarNodeId>{a, b}, r) {}
BoolAndNode::BoolAndNode(VarNodeId a, VarNodeId b, bool shouldHold)
    : ViolationInvariantNode(std::vector<VarNodeId>{a, b},
                             shouldHold) {}

void BoolAndNode::registerOutputVars(InvariantGraph& invariantGraph,
                                     propagation::SolverBase& solver) {
  if (violationVarId(invariantGraph) == propagation::NULL_ID) {
    if (shouldHold()) {
      registerViolation(invariantGraph, solver);
    } else {
      assert(!isReified());
      _intermediate = solver.makeIntVar(0, 0, 0);
      setViolationVarId(invariantGraph,
                        solver.makeIntView<propagation::NotEqualConst>(
                            solver, _intermediate, 0));
    }
  }
}

void BoolAndNode::registerNode(InvariantGraph& invariantGraph,
                               propagation::SolverBase& solver) {
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);
  assert(invariantGraph.varId(a()) != propagation::NULL_ID);
  assert(invariantGraph.varId(b()) != propagation::NULL_ID);

  solver.makeInvariant<propagation::BoolAnd>(
      solver, violationVarId(invariantGraph), invariantGraph.varId(a()),
      invariantGraph.varId(b()));
}

}  // namespace atlantis::invariantgraph