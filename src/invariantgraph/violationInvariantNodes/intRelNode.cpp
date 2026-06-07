#include "atlantis/invariantgraph/violationInvariantNodes/intRelNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {

IntRelNode::IntRelNode(InvariantGraph& graph, const VarNodeId a,
                       const RelationType relType, const VarNodeId b,
                       const VarNodeId r)
    : ViolationInvariantNode(graph, {a, b}, r), _relType(relType) {}

IntRelNode::IntRelNode(InvariantGraph& graph, const VarNodeId a,
                       const RelationType relType, const VarNodeId b,
                       const bool shouldHold)
    : ViolationInvariantNode(graph, {a, b}, shouldHold), _relType(relType) {}

void IntRelNode::init(const InvariantNodeId id) {
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

void IntRelNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  if (isReified()) {
    return constraintSolver().int_rel_reif(
        staticInputVarNodeConst(0).constraintVarId(), _relType,
        staticInputVarNodeConst(1).constraintVarId(),
        reifiedVarNode().constraintVarId());
  }
  constraintSolver().int_rel(
      staticInputVarNodeConst(0).constraintVarId(), _relType,
      staticInputVarNodeConst(1).constraintVarId(), shouldHold());
}

void IntRelNode::updateState() {
  ViolationInvariantNode::updateState();
  if (!isReified() && !shouldHold()) {
    _relType = invertRelationType(_relType);
  }
  if (a() == b()) {
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (isReified()) {
    return;
  }
  if (_relType == RelationType::REL_TYPE_GT) {
    if (varNodeConst(a()).lowerBound() > varNodeConst(b()).upperBound()) {
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
  }
  if (_relType == RelationType::REL_TYPE_GE) {
    if (varNodeConst(a()).lowerBound() >= varNodeConst(b()).upperBound()) {
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
  }
  if (_relType == RelationType::REL_TYPE_EQ) {
    if (varNodeConst(a()).isFixed() || varNodeConst(b()).isFixed()) {
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
  }
  if (_relType == RelationType::REL_TYPE_NE) {
    if (varNodeConst(a()).upperBound() < varNodeConst(b()).lowerBound() ||
        varNodeConst(a()).lowerBound() > varNodeConst(b()).upperBound()) {
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
  }
  if (_relType == RelationType::REL_TYPE_LE) {
    if (varNodeConst(a()).upperBound() <= varNodeConst(b()).lowerBound()) {
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
  }
  if (_relType == RelationType::REL_TYPE_LT) {
    if (varNodeConst(a()).upperBound() < varNodeConst(b()).lowerBound()) {
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
  }
}

void IntRelNode::registerOutputVars(propagation::SolverBase& solver,
                                    SolverMapping& mapping) const {
  registerViolation(solver, mapping);
  assert(std::ranges::all_of(outputVarNodeIds(), [&](const VarNodeId vId) {
    return mapping.solverId(vId) != propagation::NULL_ID;
  }));
}

void IntRelNode::registerNode(propagation::SolverBase& solver,
                              SolverMapping& mapping) const {
  assert(violationVarId(mapping) != propagation::NULL_ID);
  assert(violationVarId(mapping).isVar());
  assert(shouldHold());

  makeSolverBoolRelation(solver, mapping.solverId(a()), _relType,
                         mapping.solverId(b()), violationVarId(mapping),
                         shouldHold());
}

std::string IntRelNode::dotLangIdentifier() const { return "int_le_node"; }

}  // namespace atlantis::invariantgraph
