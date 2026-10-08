#include "atlantis/invariantgraph/violationInvariantNodes/intRelNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {

IntRelNode::IntRelNode(InvariantGraph& graph, VarNode& a,
                       const RelationType relType, VarNode& b, const VarNode& r)
    : ViolationInvariantNode(graph, {a, b}, r), _relType(relType) {}

IntRelNode::IntRelNode(InvariantGraph& graph, VarNode& a,
                       const RelationType relType, VarNode& b,
                       const bool shouldHold)
    : ViolationInvariantNode(graph, {a, b}, shouldHold), _relType(relType) {}

void IntRelNode::init() {
  ViolationInvariantNode::init();
  assert(
      !isReified() ||
      !invariantGraphConst().varNodeConst(reifiedViolationNode()).isIntVar());
  assert(std::ranges::all_of(
      staticInputVarNodes().begin(), staticInputVarNodes().end(),
      [&](const std::shared_ptr<VarNode>& vId) { return vId.isIntVar(); }));
}

void IntRelNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  if (isReified()) {
    return constraintSolver().int_rel_reif(
        staticInputVarNodes().front().constraintVarId(), _relType,
        staticInputVarNodes().at(1).constraintVarId(),
        reifiedVarNode().constraintVarId());
  }
  constraintSolver().int_rel(
      staticInputVarNodes().front().constraintVarId(), _relType,
      staticInputVarNodes().at(1).constraintVarId(), shouldHold());
}

void IntRelNode::updateState() {
  ViolationInvariantNode::updateState();
  if (!isReified() && !shouldHold()) {
    _relType = relationTypeComplement(_relType);
    setShouldHold(true);
  }
  if (staticInputVarNodes().empty() ||
      (staticInputVarNodes().size() == 2 &&
       staticInputVarNodes().front() == staticInputVarNodes().back())) {
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  for (Int i = static_cast<Int>(staticInputVarNodes().size()) - 1; i >= 0;
       --i) {
    if (staticInputVarNodes().at(i).isFixed()) {
      if (_fixedRhs.has_value()) {
        setState(InvariantNodeState::SUBSUMED);
        return;
      }
      _fixedRhs = staticInputVarNodes().at(i).lowerBound();
      if (i == 0) {
        _relType = relationTypeConverse(_relType);
      }
      removeStaticInputAtIndex(i);
    }
  }

  if (staticInputVarNodes().empty()) {
    setState(InvariantNodeState::SUBSUMED);
    return;
  }

  if (isReified()) {
    return;
  }
  const Int lhsLb = staticInputVarNodes().front().lowerBound();
  const Int lhsUb = staticInputVarNodes().front().upperBound();
  const Int rhsLb =
      _fixedRhs.has_value()
          ? *_fixedRhs
          : varNodeConst(staticInputVarNodes().back()).lowerBound();
  const Int rhsUb =
      _fixedRhs.has_value()
          ? *_fixedRhs
          : varNodeConst(staticInputVarNodes().back()).upperBound();

  if (_relType == RelationType::REL_TYPE_GT) {
    if (lhsLb > rhsUb) {
      setState(InvariantNodeState::SUBSUMED);
      staticInputVarNodes().front().tightenDomainType(
          DomainType::DOM_LOWER_BOUND);
      if (!_fixedRhs.has_value()) {
        staticInputVarNodes().at(1).tightenDomainType(
            DomainType::DOM_UPPER_BOUND);
      }
      return;
    }
  }
  if (_relType == RelationType::REL_TYPE_GE) {
    if (lhsLb >= rhsUb) {
      setState(InvariantNodeState::SUBSUMED);
      staticInputVarNodes().front().tightenDomainType(
          DomainType::DOM_LOWER_BOUND);
      if (!_fixedRhs.has_value()) {
        staticInputVarNodes().at(1).tightenDomainType(
            DomainType::DOM_UPPER_BOUND);
      }
      return;
    }
  }
  if (_relType == RelationType::REL_TYPE_EQ) {
    if (lhsLb == lhsUb || rhsLb == rhsUb) {
      assert(lhsLb == rhsUb);
      setState(InvariantNodeState::SUBSUMED);
      staticInputVarNodes().front().tightenDomainType(DomainType::DOM_FIXED);
      if (!_fixedRhs.has_value()) {
        staticInputVarNodes().at(1).tightenDomainType(DomainType::DOM_FIXED);
      }
      return;
    }
  }
  if (_relType == RelationType::REL_TYPE_NE) {
    if (lhsUb < rhsLb || lhsLb > rhsUb) {
      setState(InvariantNodeState::SUBSUMED);
      staticInputVarNodes().front().tightenDomainType();
      if (!_fixedRhs.has_value()) {
        staticInputVarNodes().at(1).tightenDomainType();
      }
      return;
    }
  }
  if (_relType == RelationType::REL_TYPE_LE) {
    if (lhsUb <= rhsLb) {
      setState(InvariantNodeState::SUBSUMED);
      staticInputVarNodes().front().tightenDomainType(
          DomainType::DOM_UPPER_BOUND);
      if (!_fixedRhs.has_value()) {
        staticInputVarNodes().at(1).tightenDomainType(
            DomainType::DOM_LOWER_BOUND);
      }
      return;
    }
  }
  if (_relType == RelationType::REL_TYPE_LT) {
    if (lhsUb < rhsLb) {
      setState(InvariantNodeState::SUBSUMED);
      staticInputVarNodes().front().tightenDomainType(
          DomainType::DOM_UPPER_BOUND);
      if (!_fixedRhs.has_value()) {
        staticInputVarNodes().at(1).tightenDomainType(
            DomainType::DOM_LOWER_BOUND);
      }
    }
  }
}

bool IntRelNode::canBeReplaced() const {
  if (state() != InvariantNodeState::ACTIVE) {
    return false;
  }
  return !isReified() && staticInputVarNodes().size() > 1 &&
         ((_relType == RelationType::REL_TYPE_EQ && shouldHold()) ||
          (_relType == RelationType::REL_TYPE_NE && !shouldHold()));
}

bool IntRelNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  VarNode& frontVarNodeId = staticInputVarNodes().front();
  for (size_t i = 1; i < staticInputVarNodes().size(); i++) {
    if (staticInputVarNodes().at(i) != frontVarNodeId) {
      invariantGraph().replaceVarNode(staticInputVarNodes().at(i),
                                      frontVarNodeId);
    }
  }
  return true;
}

void IntRelNode::registerOutputVars(propagation::SolverBase& solver,
                                    SolverMapping& mapping) const {
  if (staticInputVarNodes().empty()) {
    return;
  }
  if (staticInputVarNodes().size() == 1) {
    assert(_fixedRhs.has_value());
    setViolationVarId(
        makeSolverConstIntRelation(
            solver, mapping.solverId(staticInputVarNodes().front()), _relType,
            *_fixedRhs, shouldHold()),
        mapping);
  } else {
    assert(staticInputVarNodes().size() == 2);
    registerViolation(solver, mapping);
  }
  assert(std::ranges::all_of(
      outputVarNodes(), [&](const std::shared_ptr<VarNode>& vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void IntRelNode::registerNode(propagation::SolverBase& solver,
                              SolverMapping& mapping) const {
  if (staticInputVarNodes().size() <= 1) {
    assert(staticInputVarNodes().empty() ? true
                                         : violationVarId(mapping).isView());
    return;
  }
  assert(staticInputVarNodes().size() == 2);
  assert(violationVarId(mapping) != propagation::NULL_ID);
  assert(violationVarId(mapping).isVar());
  assert(shouldHold());

  makeSolverIntRelation(solver, mapping.solverId(staticInputVarNodes().front()),
                        _relType,
                        mapping.solverId(staticInputVarNodes().back()),
                        violationVarId(mapping), shouldHold());
}

std::string IntRelNode::dotLangIdentifier() const {
  return std::string{"int_"} + relToAcronym(_relType) +
         (isReified() ? "_reif" : "");
}

}  // namespace atlantis::invariantgraph
