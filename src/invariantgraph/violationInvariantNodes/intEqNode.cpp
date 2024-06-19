#include "atlantis/invariantgraph/violationInvariantNodes/intEqNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/allDifferentNode.hpp"
#include "atlantis/propagation/violationInvariants/equal.hpp"
#include "atlantis/propagation/violationInvariants/notEqual.hpp"

namespace atlantis::invariantgraph {

IntEqNode::IntEqNode(VarNodeId a, VarNodeId b, VarNodeId r, bool breaksCycle)
    : ViolationInvariantNode({a, b}, r), _breaksCycle(breaksCycle) {}

IntEqNode::IntEqNode(VarNodeId a, VarNodeId b, bool shouldHold,
                     bool breaksCycle)
    : ViolationInvariantNode({a, b}, shouldHold), _breaksCycle(breaksCycle) {}

void IntEqNode::registerOutputVars(InvariantGraph& invariantGraph,
                                   propagation::SolverBase& solver) {
  registerViolation(invariantGraph, solver);
}

bool IntEqNode::canBeReplaced(const InvariantGraph& invariantGraph) const {
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

bool IntEqNode::replace(InvariantGraph& invariantGraph) {
  if (!canBeReplaced(invariantGraph)) {
    return false;
  }
  if (!shouldHold()) {
    invariantGraph.addInvariantNode(
        std::make_unique<AllDifferentNode>(a(), b()));
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

void IntEqNode::registerNode(InvariantGraph& invariantGraph,
                             propagation::SolverBase& solver) {
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);
  assert(invariantGraph.varId(a()) != propagation::NULL_ID);
  assert(invariantGraph.varId(b()) != propagation::NULL_ID);

  if (shouldHold()) {
    solver.makeViolationInvariant<propagation::Equal>(
        solver, violationVarId(invariantGraph), invariantGraph.varId(a()),
        invariantGraph.varId(b()));
  } else {
    assert(!isReified());
    solver.makeViolationInvariant<propagation::NotEqual>(
        solver, violationVarId(invariantGraph), invariantGraph.varId(a()),
        invariantGraph.varId(b()));
  }
}

}  // namespace atlantis::invariantgraph
