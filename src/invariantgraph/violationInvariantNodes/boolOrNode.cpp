#include "invariantgraph/violationInvariantNodes/boolOrNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

BoolOrNode::BoolOrNode(VarNodeId a, VarNodeId b, VarNodeId r)
    : ViolationInvariantNode(std::move(std::vector<VarNodeId>{a, b}), r) {}
BoolOrNode::BoolOrNode(VarNodeId a, VarNodeId b, bool shouldHold)
    : ViolationInvariantNode(std::move(std::vector<VarNodeId>{a, b}),
                             shouldHold) {}

void BoolOrNode::registerOutputVars(InvariantGraph& invariantGraph,
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

void BoolOrNode::registerNode(InvariantGraph& invariantGraph,
                              propagation::SolverBase& solver) {
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);

  solver.makeInvariant<propagation::BoolOr>(
      solver, violationVarId(invariantGraph), invariantGraph.varId(a()),
      invariantGraph.varId(b()));
}

}  // namespace atlantis::invariantgraph