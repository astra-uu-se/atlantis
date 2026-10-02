#include "atlantis/invariantgraph/violationInvariantNodes/boolRelNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {
class VarNode;

BoolRelNode::BoolRelNode(InvariantGraph& graph, VarNode& a,
                         const RelationType relType, VarNode& b,
                         const VarNode& r)
    : ViolationInvariantNode(graph, std::vector<std::shared_ptr<VarNode>>{a, b},
                             r),
      _relType(relType) {}

BoolRelNode::BoolRelNode(InvariantGraph& graph, VarNode& a,
                         const RelationType relType, VarNode& b,
                         const bool shouldHold)
    : ViolationInvariantNode(graph, std::vector<std::shared_ptr<VarNode>>{a, b},
                             shouldHold),
      _relType(relType) {}

void BoolRelNode::init() {
  ViolationInvariantNode::init();
  assert(
      !isReified() ||
      !invariantGraphConst().varNodeConst(reifiedViolationNode()).isIntVar());
  assert(std::ranges::none_of(
      staticInputVarNodes().begin(), staticInputVarNodes().end(),
      [&](const std::shared_ptr<VarNode>& vId) { return vId.isIntVar(); }));
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

  if (staticInputVarNodes().empty()) {
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
         staticInputVarNodes().size() > 1 &&
         ((_relType == RelationType::REL_TYPE_EQ && shouldHold()) ||
          (_relType == RelationType::REL_TYPE_NE && !shouldHold()));
}

bool BoolRelNode::replace() {
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

void BoolRelNode::registerOutputVars(propagation::SolverBase& solver,
                                     SolverMapping& mapping) const {
  if (staticInputVarNodes().empty()) {
    return;
  }
  if (staticInputVarNodes().size() == 1) {
    assert(_fixedRhs.has_value());
    setViolationVarId(
        makeSolverConstBoolRelation(
            solver, mapping.solverId(staticInputVarNodes().front()), _relType,
            *_fixedRhs),
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

void BoolRelNode::registerNode(propagation::SolverBase& solver,
                               SolverMapping& mapping) const {
  if (staticInputVarNodes().size() <= 1) {
    assert(staticInputVarNodes().empty() ? true
                                         : violationVarId(mapping).isView());
    return;
  }
  assert(staticInputVarNodes().size() == 2);
  assert(violationVarId(mapping) != propagation::NULL_ID);
  assert(violationVarId(mapping).isVar());

  makeSolverBoolRelation(
      solver, mapping.solverId(staticInputVarNodes().front()), _relType,
      mapping.solverId(staticInputVarNodes().back()), violationVarId(mapping),
      shouldHold());
}

std::string BoolRelNode::dotLangIdentifier() const {
  return std::string{"bool_"} + relToAcronym(_relType) +
         (isReified() ? "_reif" : "");
}

}  // namespace atlantis::invariantgraph
