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
                         VarNode& r)
    : ViolationInvariantNode(graph, std::vector<std::shared_ptr<VarNode>>{a.ptr(), b.ptr()},
                             r),
      _relType(relType) {}

BoolRelNode::BoolRelNode(InvariantGraph& graph, VarNode& a,
                         const RelationType relType, VarNode& b,
                         const bool shouldHold)
    : ViolationInvariantNode(graph, std::vector<std::shared_ptr<VarNode>>{a.ptr(), b.ptr()},
                             shouldHold),
      _relType(relType) {}

void BoolRelNode::init() {
  ViolationInvariantNode::init();
  assert(
      !isReified() ||
      reifiedViolationNode()->isIntVar());
  assert(std::ranges::none_of(
      staticInputVarNodes().begin(), staticInputVarNodes().end(),
      [&](const std::shared_ptr<VarNode>& vNode) { return vNode->isIntVar(); }));
}

void BoolRelNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  if (isReified()) {
    constraintSolver().bool_rel_reif(
        staticInputVarNode(0).constraintVarId(), _relType,
        staticInputVarNode(1).constraintVarId(),
        reifiedViolationNode()->constraintVarId());
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
       &staticInputVarNode(0) == staticInputVarNodes().back().get())) {
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  for (Int i = static_cast<Int>(staticInputVarNodes().size()) - 1; i >= 0;
       --i) {
    if (staticInputVarNode(i).isFixed()) {
      if (_fixedRhs.has_value()) {
        setState(InvariantNodeState::SUBSUMED);
        return;
      }
      _fixedRhs = staticInputVarNode(i).inDomain(bool{true});
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
  VarNode& frontVarNodeId = staticInputVarNode(0);
  for (size_t i = 1; i < staticInputVarNodes().size(); i++) {
    if (&staticInputVarNode(i) != &frontVarNodeId) {
      invariantGraph().replaceVarNode(staticInputVarNode(i),
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
            solver, mapping.solverId(staticInputVarNode(0)), _relType,
            *_fixedRhs),
        mapping);
  } else {
    assert(staticInputVarNodes().size() == 2);
    registerViolation(solver, mapping);
  }
  assert(std::ranges::all_of(
      outputVarNodes(), [&](const std::shared_ptr<VarNode>& vNode) {
        return mapping.solverId(vNode) != propagation::NULL_ID;
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
      solver, mapping.solverId(staticInputVarNode(0)), _relType,
      mapping.solverId(staticInputVarNodes().back()), violationVarId(mapping),
      shouldHold());
}

std::string BoolRelNode::dotLangIdentifier() const {
  return std::string{"bool_"} + relToAcronym(_relType) +
         (isReified() ? "_reif" : "");
}

}  // namespace atlantis::invariantgraph
