#include "atlantis/invariantgraph/violationInvariantNodes/boolAllEqualNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/views/boolNotNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolAndNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolOrNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolXorNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolRelNode.hpp"
#include "atlantis/propagation/invariants/boolXor.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/propagation/violationInvariants/boolAllEqual.hpp"
#include "atlantis/propagation/violationInvariants/boolEqual.hpp"

namespace atlantis::invariantgraph {

BoolAllEqualNode::BoolAllEqualNode(InvariantGraph& graph, VarNode& a,
                                   VarNode& b, VarNode& r,
                                   const bool breaksCycle)
    : BoolAllEqualNode(graph, std::vector<std::shared_ptr<VarNode>>{a.ptr(), b.ptr()}, r,
                       breaksCycle) {}

BoolAllEqualNode::BoolAllEqualNode(InvariantGraph& graph, VarNode& a,
                                   VarNode& b, const bool shouldHold,
                                   const bool breaksCycle)
    : BoolAllEqualNode(graph, std::vector<std::shared_ptr<VarNode>>{a.ptr(), b.ptr()},
                       shouldHold, breaksCycle) {}

BoolAllEqualNode::BoolAllEqualNode(InvariantGraph& graph,
                                   std::vector<std::shared_ptr<VarNode>>&& vars,
                                   VarNode& r, const bool breaksCycle)
    : ViolationInvariantNode(graph, std::move(vars), r),
      _breaksCycle(breaksCycle) {}

BoolAllEqualNode::BoolAllEqualNode(InvariantGraph& graph,
                                   std::vector<std::shared_ptr<VarNode>>&& vars,
                                   const bool shouldHold,
                                   const bool breaksCycle)
    : ViolationInvariantNode(graph, std::move(vars), shouldHold),
      _breaksCycle(breaksCycle) {}

void BoolAllEqualNode::init() {
  ViolationInvariantNode::init();
  assert(
      !isReified() ||
      !reifiedViolationNode()->isIntVar());
  assert(std::ranges::none_of(
      staticInputVarNodes().begin(), staticInputVarNodes().end(),
      [&](const std::shared_ptr<VarNode>& vNode) { return vNode->isIntVar(); }));
}

void BoolAllEqualNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  if (staticInputVarNodes().size() < 2) {
    return;
  }
  if (staticInputVarNodes().size() == 2) {
    if (isReified()) {
      constraintSolver().bool_eq_reif(
          staticInputVarNode(0).constraintVarId(),
          staticInputVarNode(1).constraintVarId(),
          reifiedViolationNode()->constraintVarId());
    } else {
      constraintSolver().bool_eq(
          staticInputVarNode(0).constraintVarId(),
          staticInputVarNode(1).constraintVarId(), shouldHold());
    }
    return;
  }
  std::vector<Int> coeffs(staticInputVarNodes().size(), -1);
  coeffs.front() = static_cast<Int>(staticInputVarNodes().size()) - 1;

  std::vector<ConstraintVarId> inputs(staticInputVarNodes().size(),
                                      ConstraintVarId{NULL_NODE_ID});
  for (size_t i = 0; i < staticInputVarNodes().size(); i++) {
    inputs[i] = staticInputVarNode(i).constraintVarId();
  }
  if (isReified()) {
    constraintSolver().bool_lin_reif(coeffs, inputs, RelationType::REL_TYPE_EQ,
                                     0,
                                     reifiedViolationNode()->constraintVarId());
  } else {
    constraintSolver().bool_lin(coeffs, inputs, RelationType::REL_TYPE_EQ, 0,
                                shouldHold());
  }
}

void BoolAllEqualNode::updateState() {
  ViolationInvariantNode::updateState();

  if (staticInputVarNodes().size() == 1 && !isReified()) {
    setState(InvariantNodeState::SUBSUMED);
    return;
  }

  std::vector<std::shared_ptr<VarNode>> varsToRemove;
  varsToRemove.reserve(staticInputVarNodes().size());

  for (const auto& vNode : staticInputVarNodes()) {
    if (vNode->isFixed()) {
      assert(!_fixedVal.has_value() ||
             *_fixedVal == vNode->lowerBound() ||
             (!isReified() && !shouldHold()));
      _fixedVal = vNode->lowerBound();
      varsToRemove.emplace_back(vNode);
    }
  }

  for (const auto& vNode : varsToRemove) {
    removeStaticInputVarNode(*vNode);
  }

  if (staticInputVarNodes().empty()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool BoolAllEqualNode::canBeReplaced() const {
  if (state() != InvariantNodeState::ACTIVE) {
    return false;
  }
  if (staticInputVarNodes().size() == 1) {
    assert(isReified());
    assert(_fixedVal.has_value());
    return true;
  }
  return !_breaksCycle && !isReified() &&
         (shouldHold() || staticInputVarNodes().size() == 2);
}

bool BoolAllEqualNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (staticInputVarNodes().size() == 1) {
    assert(isReified());
    assert(_fixedVal.has_value());
    if (_fixedVal.has_value() && *_fixedVal) {
      invariantGraph().replaceVarNode(*reifiedViolationNode(),
                                      staticInputVarNode(0));
    } else {
      invariantGraph().addInvariantNode(std::make_shared<BoolNotNode>(
          invariantGraph(), staticInputVarNode(0),
          *reifiedViolationNode()));
    }
    return true;
  }
  assert(!isReified());
  assert(!_breaksCycle);
  if (shouldHold()) {
    VarNode& frontVarId = staticInputVarNode(0);
    for (size_t i = 1; i < staticInputVarNodes().size(); ++i) {
      invariantGraph().replaceVarNode(staticInputVarNode(i), frontVarId);
    }
    return true;
  }
  assert(staticInputVarNodes().size() == 2);
  assert(!shouldHold());
  invariantGraph().addInvariantNode(std::make_shared<BoolRelNode>(
      invariantGraph(), staticInputVarNode(0),
      RelationType::REL_TYPE_NE, *staticInputVarNodes().back(), true));
  return true;
}

void BoolAllEqualNode::registerOutputVars(propagation::SolverBase& solver,
                                          SolverMapping& mapping) const {
  if (violationVarId(mapping) == propagation::NULL_ID) {
    if (shouldHold() || staticInputVarNodes().size() == 2) {
      registerViolation(solver, mapping);
      assert(mapping.intermediateId(ptrConst()) == propagation::NULL_ID);
    } else if (!shouldHold()) {
      assert(!isReified());
      mapping.setIntermediateId(ptrConst(), solver.makeIntVar(0, 0, 0));
      setViolationVarId(solver.makeIntView<propagation::NotEqualConst>(
                            solver, mapping.intermediateId(ptrConst()), 0),
                        mapping);
    }
  }
  assert(std::ranges::all_of(outputVarNodes().begin(), outputVarNodes().end(),
                             [&](const std::shared_ptr<VarNode>& vNode) {
                               return mapping.solverId(vNode) !=
                                      propagation::NULL_ID;
                             }));
}

void BoolAllEqualNode::registerNode(propagation::SolverBase& solver,
                                    SolverMapping& mapping) const {
  if (staticInputVarNodes().empty()) {
    return;
  }
  assert(violationVarId(mapping) != propagation::NULL_ID);

  std::vector<propagation::VarViewId> solverVars;
  solverVars.reserve(staticInputVarNodes().size());
  std::ranges::transform(staticInputVarNodes(), std::back_inserter(solverVars),
                         [&](const auto& id) { return mapping.solverId(id); });

  if (solverVars.size() == 2) {
    assert(mapping.intermediateId(ptrConst()) == propagation::NULL_ID);
    assert(violationVarId(mapping).isVar());
    if (shouldHold()) {
      solver.makeViolationInvariant<propagation::BoolEqual>(
          solver, violationVarId(mapping), solverVars.front(),
          solverVars.back());

    } else {
      solver.makeInvariant<propagation::BoolXor>(
          solver, violationVarId(mapping), solverVars.front(),
          solverVars.back());
    }
    return;
  }

  assert(shouldHold() !=
         (mapping.intermediateId(ptrConst()) != propagation::NULL_ID));
  assert(shouldHold() ? violationVarId(mapping).isVar()
                      : violationVarId(mapping).isView());

  solver.makeViolationInvariant<propagation::BoolAllEqual>(
      solver,
      mapping.intermediateId(ptrConst()) == propagation::NULL_ID
          ? violationVarId(mapping)
          : mapping.intermediateId(ptrConst()),
      std::move(solverVars));
}

std::string BoolAllEqualNode::dotLangIdentifier() const {
  return "bool_all_equal";
}

}  // namespace atlantis::invariantgraph
