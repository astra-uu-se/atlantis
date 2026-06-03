#include "atlantis/invariantgraph/violationInvariantNodes/boolLeNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/views/boolNotNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/violationInvariants/boolLessEqual.hpp"
#include "atlantis/propagation/violationInvariants/boolLessThan.hpp"

namespace atlantis::invariantgraph {
class VarNode;

BoolLeNode::BoolLeNode(InvariantGraph& graph, const VarNodeId a,
                       const VarNodeId b, const VarNodeId r)
    : ViolationInvariantNode(graph, std::vector<VarNodeId>{a, b}, r) {}

BoolLeNode::BoolLeNode(InvariantGraph& graph, const VarNodeId a,
                       const VarNodeId b, const bool shouldHold)
    : ViolationInvariantNode(graph, std::vector<VarNodeId>{a, b}, shouldHold) {}

void BoolLeNode::init(const InvariantNodeId id) {
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

void BoolLeNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  if (isReified()) {
    constraintSolver().bool_le_reif(staticInputVarNode(0).constraintVarId(),
                                    staticInputVarNode(1).constraintVarId(),
                                    reifiedVarNodeConst().constraintVarId());
  } else {
    constraintSolver().bool_le(staticInputVarNode(0).constraintVarId(),
                               staticInputVarNode(1).constraintVarId(),
                               shouldHold());
  }
}

void BoolLeNode::updateState() {
  ViolationInvariantNode::updateState();
  if (staticInputVarNodeIds().size() < 2) {
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (a() == b()) {
    if (isReified()) {
      fixReified(true);
    } else if (!shouldHold()) {
      throw InconsistencyException("BoolLeNode neg: a == b");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if ((varNode(a()).isFixed() && varNode(b()).isFixed()) ||
      ((varNode(a()).isFixed() || varNode(b()).isFixed()) && !isReified())) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool BoolLeNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         staticInputVarNodeIds().size() == 2 && isReified() &&
         invariantGraphConst().varNodeConst(a()).isFixed() !=
             invariantGraphConst().varNodeConst(b()).isFixed();
}

bool BoolLeNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  assert(isReified());
  if (invariantGraph().varNode(a()).isFixed()) {
    assert(invariantGraph().varNode(a()).inDomain(bool{true}));
    invariantGraph().replaceVarNode(reifiedViolationNodeId(), b());
  } else {
    assert(invariantGraph().varNode(b()).isFixed() &&
           invariantGraph().varNode(b()).inDomain(bool{false}));
    invariantGraph().addInvariantNode(std::make_shared<BoolNotNode>(
        invariantGraph(), a(), reifiedViolationNodeId()));
  }
  return true;
}

void BoolLeNode::registerOutputVars(propagation::SolverBase& solver,
                                    SolverMapping& mapping) const {
  registerViolation(solver, mapping);
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void BoolLeNode::registerNode(propagation::SolverBase& solver,
                              SolverMapping& mapping) const {
  assert(violationVarId(mapping) != propagation::NULL_ID);
  assert(mapping.solverId(a()) != propagation::NULL_ID);
  assert(mapping.solverId(b()) != propagation::NULL_ID);
  assert(violationVarId(mapping).isVar());

  if (shouldHold()) {
    solver.makeViolationInvariant<propagation::BoolLessEqual>(
        solver, violationVarId(mapping), mapping.solverId(a()),
        mapping.solverId(b()));
  } else {
    assert(!isReified());
    solver.makeViolationInvariant<propagation::BoolLessThan>(
        solver, violationVarId(mapping), mapping.solverId(b()),
        mapping.solverId(a()));
  }
}

std::string BoolLeNode::dotLangIdentifier() const { return "bool_le"; }

}  // namespace atlantis::invariantgraph
