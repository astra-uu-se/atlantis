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
    _relType = relationTypeComplement(_relType);
    setShouldHold(true);
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
      _fixedRhs = staticInputVarNodeConst(i).lowerBound();
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
  const Int lhsLb = staticInputVarNodeConst(0).lowerBound();
  const Int lhsUb = staticInputVarNodeConst(0).upperBound();
  const Int rhsLb =
      _fixedRhs.has_value()
          ? *_fixedRhs
          : varNodeConst(staticInputVarNodeIds().back()).lowerBound();
  const Int rhsUb =
      _fixedRhs.has_value()
          ? *_fixedRhs
          : varNodeConst(staticInputVarNodeIds().back()).upperBound();

  if (_relType == RelationType::REL_TYPE_GT) {
    if (lhsLb > rhsUb) {
      setState(InvariantNodeState::SUBSUMED);
      staticInputVarNode(0).tightenDomainType(DomainType::DOM_LOWER_BOUND);
      if (!_fixedRhs.has_value()) {
        staticInputVarNode(1).tightenDomainType(DomainType::DOM_UPPER_BOUND);
      }
      return;
    }
  }
  if (_relType == RelationType::REL_TYPE_GE) {
    if (lhsLb >= rhsUb) {
      setState(InvariantNodeState::SUBSUMED);
      staticInputVarNode(0).tightenDomainType(DomainType::DOM_LOWER_BOUND);
      if (!_fixedRhs.has_value()) {
        staticInputVarNode(1).tightenDomainType(DomainType::DOM_UPPER_BOUND);
      }
      return;
    }
  }
  if (_relType == RelationType::REL_TYPE_EQ) {
    if (lhsLb == lhsUb || rhsLb == rhsUb) {
      assert(lhsLb == rhsUb);
      setState(InvariantNodeState::SUBSUMED);
      staticInputVarNode(0).tightenDomainType(DomainType::DOM_FIXED);
      if (!_fixedRhs.has_value()) {
        staticInputVarNode(1).tightenDomainType(DomainType::DOM_FIXED);
      }
      return;
    }
  }
  if (_relType == RelationType::REL_TYPE_NE) {
    if (lhsUb < rhsLb || lhsLb > rhsUb) {
      setState(InvariantNodeState::SUBSUMED);
      staticInputVarNode(0).tightenDomainType();
      if (!_fixedRhs.has_value()) {
        staticInputVarNode(1).tightenDomainType();
      }
      return;
    }
  }
  if (_relType == RelationType::REL_TYPE_LE) {
    if (lhsUb <= rhsLb) {
      setState(InvariantNodeState::SUBSUMED);
      staticInputVarNode(0).tightenDomainType(DomainType::DOM_UPPER_BOUND);
      if (!_fixedRhs.has_value()) {
        staticInputVarNode(1).tightenDomainType(DomainType::DOM_LOWER_BOUND);
      }
      return;
    }
  }
  if (_relType == RelationType::REL_TYPE_LT) {
    if (lhsUb < rhsLb) {
      setState(InvariantNodeState::SUBSUMED);
      staticInputVarNode(0).tightenDomainType(DomainType::DOM_UPPER_BOUND);
      if (!_fixedRhs.has_value()) {
        staticInputVarNode(1).tightenDomainType(DomainType::DOM_LOWER_BOUND);
      }
      return;
    }
  }
}

bool IntRelNode::canBeReplaced() const {
  if (state() != InvariantNodeState::ACTIVE) {
    return false;
  }
  return !isReified() && staticInputVarNodeIds().size() > 1 &&
         ((_relType == RelationType::REL_TYPE_EQ && shouldHold()) ||
          (_relType == RelationType::REL_TYPE_NE && !shouldHold()));
}

bool IntRelNode::replace() {
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

void IntRelNode::registerOutputVars(propagation::SolverBase& solver,
                                    SolverMapping& mapping) const {
  if (staticInputVarNodeIds().empty()) {
    return;
  }
  if (staticInputVarNodeIds().size() == 1) {
    assert(_fixedRhs.has_value());
    setViolationVarId(
        makeSolverConstIntRelation(
            solver, mapping.solverId(staticInputVarNodeIds().front()), _relType,
            *_fixedRhs, shouldHold()),
        mapping);
  } else {
    assert(staticInputVarNodeIds().size() == 2);
    registerViolation(solver, mapping);
  }
  assert(std::ranges::all_of(outputVarNodeIds(), [&](const VarNodeId vId) {
    return mapping.solverId(vId) != propagation::NULL_ID;
  }));
}

void IntRelNode::registerNode(propagation::SolverBase& solver,
                              SolverMapping& mapping) const {
  if (staticInputVarNodeIds().size() <= 1) {
    assert(staticInputVarNodeIds().empty() ? true
                                           : violationVarId(mapping).isView());
    return;
  }
  assert(staticInputVarNodeIds().size() == 2);
  assert(violationVarId(mapping) != propagation::NULL_ID);
  assert(violationVarId(mapping).isVar());
  assert(shouldHold());

  makeSolverIntRelation(
      solver, mapping.solverId(staticInputVarNodeIds().front()), _relType,
      mapping.solverId(staticInputVarNodeIds().back()), violationVarId(mapping),
      shouldHold());
}

std::string IntRelNode::dotLangIdentifier() const {
  return std::string{"int_"} + relToAcronym(_relType) +
         (isReified() ? "_reif" : "");
}

}  // namespace atlantis::invariantgraph
