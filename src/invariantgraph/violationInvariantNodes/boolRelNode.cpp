#include "atlantis/invariantgraph/violationInvariantNodes/boolRelNode.hpp"

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

BoolRelNode::BoolRelNode(InvariantGraph& graph, const VarNodeId a,
                         const RelationType relType, const VarNodeId b,
                         const VarNodeId r)
    : ViolationInvariantNode(graph, std::vector<VarNodeId>{a, b}, r),
      _relType(relType) {}

BoolRelNode::BoolRelNode(InvariantGraph& graph, const VarNodeId a,
                         const RelationType relType, const VarNodeId b,
                         const bool shouldHold)
    : ViolationInvariantNode(graph, std::vector<VarNodeId>{a, b}, shouldHold),
      _relType(relType) {}

void BoolRelNode::init(const InvariantNodeId id) {
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

void BoolRelNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  if (isReified()) {
    constraintSolver().bool_rel_reif(staticInputVarNode(0).constraintVarId(),
                                     _relType,
                                     staticInputVarNode(1).constraintVarId(),
                                     reifiedVarNodeConst().constraintVarId());
  } else {
    constraintSolver().bool_rel(
        staticInputVarNode(0).constraintVarId(), _relType,
        staticInputVarNode(1).constraintVarId(), shouldHold());
  }
}

void BoolRelNode::updateState() {
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
  if (_relType == RelationType::REL_TYPE_LE ||
      _relType == RelationType::REL_TYPE_GT ||
      _relType == RelationType::REL_TYPE_NE ||
      _relType == RelationType::REL_TYPE_EQ) {
    if (staticInputVarNodeConst(0).isFixed() ||
        staticInputVarNodeConst(1).isFixed()) {
      setState(InvariantNodeState::SUBSUMED);
    }
  } else {
    assert(_relType == RelationType::REL_TYPE_LT ||
           _relType == RelationType::REL_TYPE_GE);
    if (staticInputVarNodeConst(0).isFixed() &&
        staticInputVarNodeConst(1).isFixed()) {
      setState(InvariantNodeState::SUBSUMED);
    }
  }
}

void BoolRelNode::registerOutputVars(propagation::SolverBase& solver,
                                     SolverMapping& mapping) const {
  registerViolation(solver, mapping);
  assert(std::ranges::all_of(outputVarNodeIds(), [&](const VarNodeId vId) {
    return mapping.solverId(vId) != propagation::NULL_ID;
  }));
}

void BoolRelNode::registerNode(propagation::SolverBase& solver,
                               SolverMapping& mapping) const {
  assert(violationVarId(mapping) != propagation::NULL_ID);
  assert(violationVarId(mapping).isVar());

  assert(mapping.solverId(a()) != propagation::NULL_ID);
  assert(mapping.solverId(b()) != propagation::NULL_ID);

  makeSolverBoolRelation(solver, mapping.solverId(a()), _relType,
                         mapping.solverId(b()), violationVarId(mapping),
                         shouldHold());
}

std::string BoolRelNode::dotLangIdentifier() const { return "bool_lt"; }

}  // namespace atlantis::invariantgraph
