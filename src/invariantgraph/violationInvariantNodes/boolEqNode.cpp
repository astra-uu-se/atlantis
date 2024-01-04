#include "invariantgraph/violationInvariantNodes/boolEqNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

BoolEqNode::BoolEqNode(VarNodeId a, VarNodeId b, VarNodeId r)
    : ViolationInvariantNode(std::move(std::vector<VarNodeId>{a, b}), r) {}

BoolEqNode::BoolEqNode(VarNodeId a, VarNodeId b, bool shouldHold)
    : ViolationInvariantNode(std::move(std::vector<VarNodeId>{a, b}),
                             shouldHold) {}

BoolEqNode::Prune(InvariantGraph& invariantGraph) {
  VarNode& aNode = invariantGraph.varNode(a());
  VarNode& bNode = invariantGraph.varNode(b());
  if (aNode.isFixed()) {
    bNode.removeValue(aNode.domain().lowerBound() != 0);
  }
  if (bNode.isFixed()) {
    aNode.removeValue(bNode.domain().lowerBound() != 0);
  }
  if (isReified() && aNode.isFixed() && bNode.isFixed()) {
    }
}

bool BoolEqNode::canBeRemoved(cibst InvariantGraph& invariantGraph) const {
  return invariantGraph.varNodeConst(a()).isFixed() ||
         invariantGraph.varNodeConst(b()).isFixed();
}

void BoolEqNode::registerOutputVars(InvariantGraph& invariantGraph,
                                    propagation::SolverBase& solver) {
  if (violationVarId(invariantGraph) == propagation::NULL_ID) {
    _allDifferentViolationVarId = solver.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(invariantGraph,
                        solver.makeIntView<propagation::EqualConst>(
                            solver, _allDifferentViolationVarId,
                            staticInputVarNodeIds().size() - 1));
    } else {
      assert(!isReified());
      setViolationVarId(invariantGraph,
                        solver.makeIntView<propagation::NotEqualConst>(
                            solver, _allDifferentViolationVarId,
                            staticInputVarNodeIds().size() - 1));
    }
  }
}

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