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

BoolAllEqualNode::BoolAllEqualNode(InvariantGraph& graph, const VarNodeId a,
                                   const VarNodeId b, const VarNodeId r,
                                   const bool breaksCycle)
    : BoolAllEqualNode(graph, std::vector<VarNodeId>{a, b}, r, breaksCycle) {}

BoolAllEqualNode::BoolAllEqualNode(InvariantGraph& graph, const VarNodeId a,
                                   const VarNodeId b, const bool shouldHold,
                                   const bool breaksCycle)
    : BoolAllEqualNode(graph, std::vector<VarNodeId>{a, b}, shouldHold,
                       breaksCycle) {}

BoolAllEqualNode::BoolAllEqualNode(InvariantGraph& graph,
                                   std::vector<VarNodeId>&& vars,
                                   const VarNodeId r, const bool breaksCycle)
    : ViolationInvariantNode(graph, std::move(vars), r),
      _breaksCycle(breaksCycle) {}

BoolAllEqualNode::BoolAllEqualNode(InvariantGraph& graph,
                                   std::vector<VarNodeId>&& vars,
                                   const bool shouldHold,
                                   const bool breaksCycle)
    : ViolationInvariantNode(graph, std::move(vars), shouldHold),
      _breaksCycle(breaksCycle) {}

void BoolAllEqualNode::init(const InvariantNodeId id) {
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

void BoolAllEqualNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  if (staticInputVarNodeIds().size() < 2) {
    return;
  }
  if (staticInputVarNodeIds().size() == 2) {
    if (isReified()) {
      constraintSolver().bool_eq_reif(staticInputVarNode(0).constraintVarId(),
                                      staticInputVarNode(1).constraintVarId(),
                                      reifiedVarNodeConst().constraintVarId());
    } else {
      constraintSolver().bool_eq(staticInputVarNode(0).constraintVarId(),
                                 staticInputVarNode(1).constraintVarId(),
                                 shouldHold());
    }
    return;
  }
  std::vector<Int> coeffs(staticInputVarNodeIds().size(), -1);
  coeffs.front() = static_cast<Int>(staticInputVarNodeIds().size()) - 1;

  std::vector<ConstraintVarId> inputs(staticInputVarNodeIds().size(),
                                      ConstraintVarId{NULL_NODE_ID});
  for (size_t i = 0; i < staticInputVarNodeIds().size(); i++) {
    inputs[i] = staticInputVarNode(i).constraintVarId();
  }
  if (isReified()) {
    constraintSolver().bool_lin_reif(coeffs, inputs, RelationType::REL_TYPE_EQ,
                                     0,
                                     reifiedVarNodeConst().constraintVarId());
  } else {
    constraintSolver().bool_lin(coeffs, inputs, RelationType::REL_TYPE_EQ, 0,
                                shouldHold());
  }
}

void BoolAllEqualNode::updateState() {
  ViolationInvariantNode::updateState();

  if (staticInputVarNodeIds().size() == 1 && !isReified()) {
    setState(InvariantNodeState::SUBSUMED);
    return;
  }

  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());

  for (const auto vId : staticInputVarNodeIds()) {
    if (varNodeConst(vId).isFixed()) {
      assert(!_fixedVal.has_value() || *_fixedVal == varNodeConst(vId).lowerBound() || (!isReified() && !shouldHold()));
      _fixedVal = varNodeConst(vId).lowerBound();
      varsToRemove.emplace_back(vId);
    }
  }

  for (const auto vId : varsToRemove) {
    removeStaticInputVarNode(vId);
  }

  if (staticInputVarNodeIds().empty()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool BoolAllEqualNode::canBeReplaced() const {
  if (state() != InvariantNodeState::ACTIVE) {
    return false;
  }
  if (staticInputVarNodeIds().size() == 1) {
    assert(isReified());
    assert(_fixedVal.has_value());
    return true;
  }
  return !_breaksCycle && !isReified() && (shouldHold() || staticInputVarNodeIds().size() == 2);
}

bool BoolAllEqualNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (staticInputVarNodeIds().size() == 1) {
    assert(isReified());
    assert(_fixedVal.has_value());
    if (_fixedVal.has_value() && *_fixedVal) {
      invariantGraph().replaceVarNode(reifiedViolationNodeId(), staticInputVarNodeIds().front());
    } else {
      invariantGraph().addInvariantNode(std::make_shared<BoolNotNode>(invariantGraph(), staticInputVarNodeIds().front(), reifiedViolationNodeId()));
    }
    return true;
  }
  assert(!isReified());
  assert(!_breaksCycle);
  if (shouldHold()) {
    const VarNodeId frontVarId = staticInputVarNodeIds().front();
    for (size_t i = 1; i < staticInputVarNodeIds().size(); ++i) {
      invariantGraph().replaceVarNode(staticInputVarNodeIds().at(i),
                                      frontVarId);
    }
    return true;
  }
  assert(staticInputVarNodeIds().size() == 2);
  assert(!shouldHold());
  invariantGraph().addInvariantNode(std::make_shared<BoolRelNode>(
      invariantGraph(), staticInputVarNodeIds().front(), RelationType::REL_TYPE_NE, staticInputVarNodeIds().back(), true));
  return true;
}

void BoolAllEqualNode::registerOutputVars(propagation::SolverBase& solver,
                                          SolverMapping& mapping) const {
  if (violationVarId(mapping) == propagation::NULL_ID) {
    if (shouldHold() || staticInputVarNodeIds().size() == 2) {
      registerViolation(solver, mapping);
      assert(mapping.intermediateId(id()) == propagation::NULL_ID);
    } else if (!shouldHold()) {
      assert(!isReified());
      mapping.setIntermediateId(id(), solver.makeIntVar(0, 0, 0));
      setViolationVarId(solver.makeIntView<propagation::NotEqualConst>(
                            solver, mapping.intermediateId(id()), 0),
                        mapping);
    }
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void BoolAllEqualNode::registerNode(propagation::SolverBase& solver,
                                    SolverMapping& mapping) const {
  if (staticInputVarNodeIds().empty()) {
    return;
  }
  assert(violationVarId(mapping) != propagation::NULL_ID);

  std::vector<propagation::VarViewId> solverVars;
  solverVars.reserve(staticInputVarNodeIds().size());
  std::ranges::transform(staticInputVarNodeIds(),
                         std::back_inserter(solverVars),
                         [&](const auto& id) { return mapping.solverId(id); });

  if (solverVars.size() == 2) {
    assert(mapping.intermediateId(id()) == propagation::NULL_ID);
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
         (mapping.intermediateId(id()) != propagation::NULL_ID));
  assert(shouldHold() ? violationVarId(mapping).isVar()
                      : violationVarId(mapping).isView());

  solver.makeViolationInvariant<propagation::BoolAllEqual>(
      solver,
      mapping.intermediateId(id()) == propagation::NULL_ID
          ? violationVarId(mapping)
          : mapping.intermediateId(id()),
      std::move(solverVars));
}

std::string BoolAllEqualNode::dotLangIdentifier() const {
  return "bool_all_equal";
}

}  // namespace atlantis::invariantgraph
