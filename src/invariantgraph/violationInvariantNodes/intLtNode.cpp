#include "atlantis/invariantgraph/violationInvariantNodes/intLtNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
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
  if (isReified()) {
    return;
  }
  if (staticInputVarNodeIds().size() < 2) {
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (a() == b()) {
    assert(!isReified());
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  const VarNode& aNode = varNodeConst(a());
  const VarNode& bNode = varNodeConst(b());
  if (aNode.upperBound() < bNode.lowerBound() ||
      aNode.lowerBound() >= bNode.upperBound()) {
    assert((aNode.upperBound() <= bNode.lowerBound()) == shouldHold());
    setState(InvariantNodeState::SUBSUMED);
  }
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());
  for (const auto vId : staticInputVarNodeIds()) {
    if (varNodeConst(vId).isFixed()) {
      varsToRemove.emplace_back(vId);
    }
  }
  for (const auto vId : varsToRemove) {
    removeStaticInputVarNode(vId);
  }
  if (staticInputVarNodeIds().size() < 2) {
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
