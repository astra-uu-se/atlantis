#include "atlantis/invariantgraph/violationInvariantNodes/intNeNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intEqNode.hpp"
#include "atlantis/propagation/violationInvariants/equal.hpp"
#include "atlantis/propagation/violationInvariants/notEqual.hpp"

namespace atlantis::invariantgraph {

IntNeNode::IntNeNode(VarNodeId a, VarNodeId b, VarNodeId r)
    : ViolationInvariantNode({a, b}, r) {}
IntNeNode::IntNeNode(VarNodeId a, VarNodeId b, bool shouldHold)
    : ViolationInvariantNode({a, b}, shouldHold) {}

bool IntNeNode::canBeReplaced(const InvariantGraph&) const {
  if (isReified()) {
    return false;
  }
  return !shouldHold();
}

bool IntNeNode::replace(InvariantGraph& invariantGraph) {
  if (!canBeReplaced(invariantGraph)) {
    return false;
  }
  if (!shouldHold()) {
    invariantGraph.addInvariantNode(std::make_unique<IntEqNode>(a(), b()));
    return true;
  }
  return false;
}

void IntNeNode::registerOutputVars(InvariantGraph& invariantGraph,
                                   propagation::SolverBase& solver) {
  registerViolation(invariantGraph, solver);
}

void IntNeNode::registerNode(InvariantGraph& invariantGraph,
                             propagation::SolverBase& solver) {
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);

  if (shouldHold()) {
    solver.makeViolationInvariant<propagation::NotEqual>(
        solver, violationVarId(invariantGraph), invariantGraph.varId(a()),
        invariantGraph.varId(b()));
  } else {
    assert(!isReified());
    solver.makeViolationInvariant<propagation::Equal>(
        solver, violationVarId(invariantGraph), invariantGraph.varId(a()),
        invariantGraph.varId(b()));
  }
}

}  // namespace atlantis::invariantgraph
