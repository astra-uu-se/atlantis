#include "atlantis/invariantgraph/violationInvariantNodes/intLtNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/violationInvariants/lessEqual.hpp"
#include "atlantis/propagation/violationInvariants/lessThan.hpp"

namespace atlantis::invariantgraph {

IntLtNode::IntLtNode(InvariantGraph& graph, const VarNodeId a,
                     const VarNodeId b, const VarNodeId r)
    : ViolationInvariantNode(graph, {a, b}, r) {}

IntLtNode::IntLtNode(InvariantGraph& graph, const VarNodeId a,
                     const VarNodeId b, const bool shouldHold)
    : ViolationInvariantNode(graph, {a, b}, shouldHold) {}

void IntLtNode::init(const InvariantNodeId id) {
  ViolationInvariantNode::init(id);
  assert(
      !isReified() ||
      !invariantGraphConst().varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(std::ranges::all_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
}

void IntLtNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  if (isReified()) {
    return constraintSolver().int_lt_reif(
        staticInputVarNodeConst(0).constraintVarId(),
        staticInputVarNodeConst(1).constraintVarId(),
        reifiedVarNode().constraintVarId());
  }
  constraintSolver().int_lt(staticInputVarNodeConst(0).constraintVarId(),
                            staticInputVarNodeConst(1).constraintVarId(),
                            shouldHold());
}

void IntLtNode::updateState() {
  ViolationInvariantNode::updateState();
  if (staticInputVarNodeIds().size() < 2) {
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  const VarNode& aNode = invariantGraph().varNode(a());
  const VarNode& bNode = invariantGraph().varNode(b());
  if (a() == b()) {
    if (isReified()) {
      fixReified(false);
    } else if (shouldHold()) {
      throw InconsistencyException("IntLtNode: a == b");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (!isReified()) {
    if (shouldHold()) {
      // a < b
      // aNode.removeValuesAbove(bNode.upperBound() - 1);
      // bNode.removeValuesBelow(aNode.lowerBound() + 1);
    } else {
      // a >= b
      // aNode.removeValuesBelow(bNode.lowerBound());
      // bNode.removeValuesAbove(aNode.upperBound());
    }
  }
  if (aNode.upperBound() < bNode.lowerBound()) {
    // always true
    if (isReified()) {
      fixReified(true);
    } else if (!shouldHold()) {
      throw InconsistencyException("IntLtNode neg: a < b");
    }
    setState(InvariantNodeState::SUBSUMED);
  } else if (aNode.lowerBound() >= bNode.upperBound()) {
    // always false
    if (isReified()) {
      fixReified(false);
    } else if (shouldHold()) {
      throw InconsistencyException("IntLtNode: a >= b");
    }
    setState(InvariantNodeState::SUBSUMED);
  }
}

void IntLtNode::registerOutputVars(propagation::SolverBase& solver,
                                   SolverMapping& mapping) const {
  registerViolation(solver, mapping);
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void IntLtNode::registerNode(propagation::SolverBase& solver,
                             SolverMapping& mapping) const {
  assert(violationVarId(mapping) != propagation::NULL_ID);
  assert(violationVarId(mapping).isVar());

  if (shouldHold()) {
    solver.makeViolationInvariant<propagation::LessThan>(
        solver, violationVarId(mapping), mapping.solverId(a()),
        mapping.solverId(b()));
  } else {
    assert(!isReified());
    solver.makeViolationInvariant<propagation::LessEqual>(
        solver, violationVarId(mapping), mapping.solverId(b()),
        mapping.solverId(a()));
  }
}

std::string IntLtNode::dotLangIdentifier() const { return "int_lt_node"; }

}  // namespace atlantis::invariantgraph
