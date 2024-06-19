#include "atlantis/invariantgraph/violationInvariantNodes/boolEqNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolXorNode.hpp"
#include "atlantis/propagation/invariants/boolXor.hpp"
#include "atlantis/propagation/violationInvariants/boolEqual.hpp"

namespace atlantis::invariantgraph {

BoolEqNode::BoolEqNode(VarNodeId a, VarNodeId b, VarNodeId r, bool breaksCycle)
    : ViolationInvariantNode(std::vector<VarNodeId>{a, b}, r),
      _breaksCycle(breaksCycle) {}

BoolEqNode::BoolEqNode(VarNodeId a, VarNodeId b, bool shouldHold,
                       bool breaksCycle)
    : ViolationInvariantNode(std::vector<VarNodeId>{a, b}, shouldHold),
      _breaksCycle(breaksCycle) {}

bool BoolEqNode::canBeReplaced(const InvariantGraph& invariantGraph) const {
  if (isReified()) {
    return false;
  }
  if (!shouldHold()) {
    return true;
  }
  if (!_breaksCycle) {
    for (const auto& input : staticInputVarNodeIds()) {
      if (invariantGraph.varNodeConst(input).definingNodes().empty()) {
        return true;
      }
    }
  }
  return false;
}

bool BoolEqNode::replace(InvariantGraph& invariantGraph) {
  if (!canBeReplaced(invariantGraph)) {
    return false;
  }
  if (!shouldHold()) {
    invariantGraph.addInvariantNode(std::make_unique<BoolXorNode>(a(), b()));
    return true;
  }
  if (a() == b()) {
    return true;
  }
  if (invariantGraph.varNodeConst(a()).definingNodes().empty()) {
    invariantGraph.replaceVarNode(a(), b());
    return true;
  }
  if (invariantGraph.varNodeConst(b()).definingNodes().empty()) {
    invariantGraph.replaceVarNode(b(), a());
    return true;
  }
  return false;
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
