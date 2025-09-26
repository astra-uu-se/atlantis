#include "atlantis/invariantgraph/violationInvariantNodes/boolLtNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/views/boolNotNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/violationInvariants/boolLessEqual.hpp"
#include "atlantis/propagation/violationInvariants/boolLessThan.hpp"

namespace atlantis::invariantgraph {
class VarNode;

BoolLtNode::BoolLtNode(InvariantGraph& graph, VarNodeId a, VarNodeId b,
                       VarNodeId r)
    : ViolationInvariantNode(graph, std::vector<VarNodeId>{a, b}, r) {}

BoolLtNode::BoolLtNode(InvariantGraph& graph, VarNodeId a, VarNodeId b,
                       bool shouldHold)
    : ViolationInvariantNode(graph, std::vector<VarNodeId>{a, b}, shouldHold) {}

void BoolLtNode::init(InvariantNodeId id) {
  ViolationInvariantNode::init(id);
  assert(
      !isReified() ||
      !invariantGraphConst().varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(std::ranges::none_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
}

void BoolLtNode::updateState() {
  ViolationInvariantNode::updateState();
  if (staticInputVarNodeIds().size() < 2) {
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  VarNode& aNode = invariantGraph().varNode(a());
  VarNode& bNode = invariantGraph().varNode(b());
  if (a() == b()) {
    if (isReified()) {
      fixReified(false);
    } else if (shouldHold()) {
      throw InconsistencyException("BoolLtNode: a == b");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (!isReified() && shouldHold()) {
    aNode.fixToValue(bool{false});
    bNode.fixToValue(bool{true});
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (aNode.isFixed() && bNode.isFixed()) {
    const bool isSatisifed =
        aNode.inDomain(bool{false}) && bNode.inDomain(bool{true});
    if (isReified()) {
      fixReified(isSatisifed);
    } else if (isSatisifed != shouldHold()) {
      throw InconsistencyException(shouldHold() ? "BoolLtNode: a >= b"
                                                : "BoolLtNode neg: a < b");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (aNode.isFixed() || bNode.isFixed()) {
    assert(aNode.isFixed() != bNode.isFixed());
    const bool isViolated = (aNode.isFixed() && aNode.inDomain(bool{true})) ||
                            (bNode.isFixed() && bNode.inDomain(bool{false}));
    if (isViolated) {
      if (isReified()) {
        fixReified(false);
      } else if (shouldHold()) {
        throw InconsistencyException("BoolLtNode: a >= b");
      }
      setState(InvariantNodeState::SUBSUMED);
    }
  }
}

bool BoolLtNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         staticInputVarNodeIds().size() == 2 &&
         (isReified() && invariantGraphConst().varNodeConst(a()).isFixed() !=
                             invariantGraphConst().varNodeConst(b()).isFixed());
}

bool BoolLtNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  assert(isReified());
  if (invariantGraph().varNode(a()).isFixed()) {
    assert(invariantGraph().varNode(a()).inDomain(bool{false}));
    invariantGraph().replaceVarNode(reifiedViolationNodeId(), b());
  } else {
    assert(invariantGraph().varNode(b()).isFixed() &&
           invariantGraph().varNode(b()).inDomain(bool{true}));
    invariantGraph().addInvariantNode(std::make_shared<BoolNotNode>(
        invariantGraph(), a(), reifiedViolationNodeId()));
  }
  return true;
}

void BoolLtNode::registerOutputVars(propagation::SolverBase& solver,
                                    SolverMapping& mapping) const {
  registerViolation(solver, mapping);
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void BoolLtNode::registerNode(propagation::SolverBase& solver,
                              SolverMapping& mapping) const {
  assert(violationVarId(mapping) != propagation::NULL_ID);
  assert(violationVarId(mapping).isVar());

  assert(mapping.solverId(a()) != propagation::NULL_ID);
  assert(mapping.solverId(b()) != propagation::NULL_ID);

  if (shouldHold()) {
    solver.makeViolationInvariant<propagation::BoolLessThan>(
        solver, violationVarId(mapping), mapping.solverId(a()),
        mapping.solverId(b()));
  } else {
    assert(!isReified());
    solver.makeViolationInvariant<propagation::BoolLessEqual>(
        solver, violationVarId(mapping), mapping.solverId(b()),
        mapping.solverId(a()));
  }
}

std::string BoolLtNode::dotLangIdentifier() const { return "bool_lt"; }

}  // namespace atlantis::invariantgraph
