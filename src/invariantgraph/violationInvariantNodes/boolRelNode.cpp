#include "atlantis/invariantgraph/violationInvariantNodes/boolRelNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"

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
    _relType = relationTypeComplement(_relType);
  }
  if (staticInputVarNodeIds().empty() ||
      (staticInputVarNodeIds().size() == 2 &&
       staticInputVarNodeIds().front() == staticInputVarNodeIds().back())) {
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  for (Int i = static_cast<Int>(staticInputVarNodeIds().size()) - 1; i >= 0;
       --i) {
    if (staticInputVarNodeConst(i).isFixed()) {
      if (_fixedRhs.has_value()) {
        setState(InvariantNodeState::SUBSUMED);
        return;
      }
      _fixedRhs = staticInputVarNodeConst(i).inDomain(bool{true});
      if (i == 0) {
        _relType = relationTypeConverse(_relType);
      }
      removeStaticInputAtIndex(i);
    }
  }

  if (staticInputVarNodeIds().empty()) {
    setState(InvariantNodeState::SUBSUMED);
    return;
  }

  if (isReified()) {
    return;
  }
  if (_relType == RelationType::REL_TYPE_LT ||
      _relType == RelationType::REL_TYPE_GT ||
      _relType == RelationType::REL_TYPE_NE ||
      _relType == RelationType::REL_TYPE_EQ) {
    if (_fixedRhs.has_value()) {
      setState(InvariantNodeState::SUBSUMED);
    }
  } else {
    assert(_relType == RelationType::REL_TYPE_LE ||
           _relType == RelationType::REL_TYPE_GE);
    if (_fixedRhs.has_value()) {
      setState(InvariantNodeState::SUBSUMED);
    }
  }
}

bool BoolRelNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE && !isReified() &&
         staticInputVarNodeIds().size() > 1 &&
         ((_relType == RelationType::REL_TYPE_EQ && shouldHold()) ||
          (_relType == RelationType::REL_TYPE_NE && !shouldHold()));
}

bool BoolRelNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  const VarNodeId frontVarNodeId = staticInputVarNodeIds().front();
  for (size_t i = 1; i < staticInputVarNodeIds().size(); i++) {
    if (staticInputVarNodeIds().at(i) != frontVarNodeId) {
      invariantGraph().replaceVarNode(staticInputVarNodeIds().at(i),
                                      frontVarNodeId);
    }
  }
  return true;
}

void BoolRelNode::registerOutputVars(propagation::SolverBase& solver,
                                     SolverMapping& mapping) const {
  if (staticInputVarNodeIds().empty()) {
    return;
  }
  if (staticInputVarNodeIds().size() == 1) {
    assert(_fixedRhs.has_value());
    setViolationVarId(
        makeSolverConstBoolRelation(
            solver, mapping.solverId(staticInputVarNodeIds().front()), _relType,
            *_fixedRhs),
        mapping);
  } else {
    assert(staticInputVarNodeIds().size() == 2);
    registerViolation(solver, mapping);
  }
  assert(std::ranges::all_of(outputVarNodeIds(), [&](const VarNodeId vId) {
    return mapping.solverId(vId) != propagation::NULL_ID;
  }));
}

void BoolRelNode::registerNode(propagation::SolverBase& solver,
                               SolverMapping& mapping) const {
  if (staticInputVarNodeIds().size() <= 1) {
    assert(staticInputVarNodeIds().empty() ? true
                                           : violationVarId(mapping).isView());
    return;
  }
  assert(staticInputVarNodeIds().size() == 2);
  assert(violationVarId(mapping) != propagation::NULL_ID);
  assert(violationVarId(mapping).isVar());

  makeSolverBoolRelation(
      solver, mapping.solverId(staticInputVarNodeIds().front()), _relType,
      mapping.solverId(staticInputVarNodeIds().back()), violationVarId(mapping),
      shouldHold());
}

std::string BoolRelNode::dotLangIdentifier() const {
  return std::string{"bool_"} + relToAcronym(_relType) +
         (isReified() ? "_reif" : "");
}

}  // namespace atlantis::invariantgraph
