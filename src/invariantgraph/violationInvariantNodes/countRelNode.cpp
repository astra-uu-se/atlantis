#include "atlantis/invariantgraph/violationInvariantNodes/countRelNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/fzn/fzn_all_different_int.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNodes/countNode.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intRelNode.hpp"
#include "atlantis/propagation/invariants/count.hpp"
#include "atlantis/propagation/invariants/countConst.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/utils/overflow.hpp"

namespace atlantis::invariantgraph {

VarNodeId CountRelNode::needle() const {
  return _fixedNeedle.has_value() ? NULL_NODE_ID
                                  : staticInputVarNodeIds()[needleIndex()];
  ;
}

size_t CountRelNode::needleIndex() const { return numInputVars(); }

VarNodeId CountRelNode::amount() const {
  return _fixedAmount.has_value() ? NULL_NODE_ID
                                  : staticInputVarNodeIds()[amountIndex()];
  ;
}

size_t CountRelNode::amountIndex() const {
  return numInputVars() + (_fixedNeedle.has_value() ? 0 : 1);
}

size_t CountRelNode::numInputVars() const {
  return staticInputVarNodeIds().size() - (_fixedNeedle.has_value() ? 0 : 1) -
         (_fixedAmount.has_value() ? 0 : 1);
}

CountRelNode::CountRelNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars,
                           const Int needle, const Int amount,
                           const RelationType relationType,
                           const bool shouldHold)
    : ViolationInvariantNode(graph, {}, std::move(vars), shouldHold),
      _fixedNeedle(needle),
      _fixedAmount(amount),
      _relType(relationType) {}

CountRelNode::CountRelNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars,
                           const Int needle, const VarNodeId amount,
                           const RelationType relationType,
                           const bool shouldHold)
    : ViolationInvariantNode(graph, {}, append(std::move(vars), amount),
                             shouldHold),
      _fixedNeedle(needle),
      _fixedAmount(std::nullopt),
      _relType(relationType) {}

CountRelNode::CountRelNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars,
                           const VarNodeId needle, const Int amount,
                           const RelationType relationType,
                           const bool shouldHold)
    : ViolationInvariantNode(graph, {}, append(std::move(vars), needle),
                             shouldHold),
      _fixedNeedle(std::nullopt),
      _fixedAmount(amount),
      _relType(relationType) {}

CountRelNode::CountRelNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars,
                           const VarNodeId needle, const VarNodeId amount,
                           const RelationType relationType,
                           const bool shouldHold)
    : ViolationInvariantNode(graph, {},
                             append(append(std::move(vars), needle), amount),
                             shouldHold),
      _fixedNeedle(std::nullopt),
      _fixedAmount(std::nullopt),
      _relType(relationType) {}

CountRelNode::CountRelNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars,
                           const Int needle, const Int amount,
                           const RelationType relationType,
                           const VarNodeId reified)
    : ViolationInvariantNode(graph, {}, std::move(vars), reified),
      _fixedNeedle(needle),
      _fixedAmount(amount),
      _relType(relationType) {}

CountRelNode::CountRelNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars,
                           const Int needle, const VarNodeId amount,
                           const RelationType relationType,
                           const VarNodeId reified)
    : ViolationInvariantNode(graph, {}, append(std::move(vars), amount),
                             reified),
      _fixedNeedle(needle),
      _fixedAmount(std::nullopt),
      _relType(relationType) {}

CountRelNode::CountRelNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars,
                           const VarNodeId needle, const Int amount,
                           const RelationType relationType,
                           const VarNodeId reified)
    : ViolationInvariantNode(graph, {}, append(std::move(vars), needle),
                             reified),
      _fixedNeedle(std::nullopt),
      _fixedAmount(amount),
      _relType(relationType) {}

CountRelNode::CountRelNode(InvariantGraph& graph, std::vector<VarNodeId>&& vars,
                           const VarNodeId needle, const VarNodeId amount,
                           const RelationType relationType,
                           const VarNodeId reified)
    : ViolationInvariantNode(
          graph, {}, append(append(std::move(vars), needle), amount), reified),
      _fixedNeedle(std::nullopt),
      _fixedAmount(std::nullopt),
      _relType(relationType) {}

void CountRelNode::init(const InvariantNodeId id) {
  ViolationInvariantNode::init(id);
  assert(!isReified() || !reifiedVarNodeConst().isIntVar());
  assert(std::ranges::all_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const VarNodeId vId) { return varNodeConst(vId).isIntVar(); }));
}

void CountRelNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  std::vector<ConstraintVarId> inputs(numInputVars(),
                                      ConstraintVarId{NULL_NODE_ID});
  for (size_t i = 0; i < numInputVars(); ++i) {
    inputs[i] = staticInputVarNodeConst(i).constraintVarId();
  }
  assert(_offset == 0);
  if (isReified()) {
    if (_fixedNeedle.has_value()) {
      if (_fixedAmount.has_value()) {
        return constraintSolver().fzn_count_reif(
            inputs, *_fixedNeedle, _relType, *_fixedAmount,
            reifiedVarNodeConst().constraintVarId());
      }
      return constraintSolver().fzn_count_reif(
          inputs, *_fixedNeedle, _relType,
          varNodeConst(amount()).constraintVarId(),
          reifiedVarNodeConst().constraintVarId());
    }
    if (_fixedAmount.has_value()) {
      return constraintSolver().fzn_count_reif(
          inputs, varNodeConst(needle()).constraintVarId(), _relType,
          *_fixedAmount, reifiedVarNodeConst().constraintVarId());
    }
    return constraintSolver().fzn_count_reif(
        inputs, varNodeConst(needle()).constraintVarId(), _relType,
        varNodeConst(amount()).constraintVarId(),
        reifiedVarNodeConst().constraintVarId());
  }
  if (_fixedNeedle.has_value()) {
    if (_fixedAmount.has_value()) {
      return constraintSolver().fzn_count(inputs, *_fixedNeedle, _relType,
                                          *_fixedAmount, shouldHold());
    }
    return constraintSolver().fzn_count(
        inputs, *_fixedNeedle, _relType,
        varNodeConst(amount()).constraintVarId(), shouldHold());
  }
  if (_fixedAmount.has_value()) {
    return constraintSolver().fzn_count(
        inputs, varNodeConst(needle()).constraintVarId(), _relType,
        *_fixedAmount, shouldHold());
  }
  return constraintSolver().fzn_count(
      inputs, varNodeConst(needle()).constraintVarId(), _relType,
      varNodeConst(amount()).constraintVarId(), shouldHold());
}

void CountRelNode::updateState() {
  ViolationInvariantNode::updateState();
  // update fixed needle and amount
  if (!_fixedNeedle.has_value() && varNodeConst(needle()).isFixed()) {
    // updating _fixedNeedle modified indices
    const Int n = varNodeConst(needle()).lowerBound();
    removeStaticInputAtIndex(needleIndex());
    _fixedNeedle = n;
  }
  if (!_fixedAmount.has_value() && varNodeConst(amount()).isFixed()) {
    // updating _fixedAmount modified indices
    const Int a = varNodeConst(amount()).lowerBound();
    removeStaticInputAtIndex(amountIndex());
    _fixedAmount = a;
  }
  if (isReified() && !shouldHold()) {
    setShouldHold(true);
    _relType = relationTypeComplement(_relType);
  }
  std::vector<Int> indicesToRemove;
  indicesToRemove.reserve(numInputVars());
  for (Int i = static_cast<Int>(numInputVars()) - 1; i >= 0; --i) {
    if (_fixedNeedle.has_value() && staticInputVarNodeConst(i).isFixed()) {
      _offset +=
          (*_fixedNeedle == staticInputVarNodeConst(i).lowerBound() ? 1 : 0);
      indicesToRemove.emplace_back(i);
      continue;
    }
    if (!_fixedNeedle.has_value() &&
        staticInputVarNodeConst(i).constDomain()->isDisjoint(
            *varNodeConst(needle()).constDomain())) {
      indicesToRemove.emplace_back(i);
    }
  }
  for (const Int index : indicesToRemove) {
    removeStaticInputAtIndex(index);
  }
  if (staticInputVarNodeIds().empty()) {
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (numInputVars() == 0 && !isReified()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool CountRelNode::canBeReplaced() const {
  if (state() != InvariantNodeState::ACTIVE) {
    return false;
  }
  return !isReified() && !_fixedAmount.has_value() &&
         (shouldHold() ? _relType : relationTypeComplement(_relType)) ==
             RelationType::REL_TYPE_EQ;
}

bool CountRelNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  std::vector<VarNodeId> varNodeIds(numInputVars());
  for (size_t i = 0; i < numInputVars(); ++i) {
    varNodeIds[i] = staticInputVarNodeIds()[i];
  }
  if (_fixedNeedle.has_value()) {
    invariantGraph().addInvariantNode(std::make_shared<CountNode>(
        invariantGraph(), std::move(varNodeIds), *_fixedNeedle, amount()));
  } else {
    invariantGraph().addInvariantNode(std::make_shared<CountNode>(
        invariantGraph(), std::move(varNodeIds), needle(), amount()));
  }
  return true;
}

void CountRelNode::registerOutputVars(propagation::SolverBase& solver,
                                      SolverMapping& mapping) const {
  if (violationVarId(mapping) == propagation::NULL_ID) {
    if (numInputVars() == 0 && isReified()) {
      assert(!_fixedAmount.has_value());
      setViolationVarId(solverConstRelation(solver, mapping.solverId(amount()),
                                            _offset, _relType, true, true),
                        mapping);
    } else {
      assert(isReified() ||
             (shouldHold() ? _relType : relationTypeComplement(_relType)) !=
                 RelationType::REL_TYPE_EQ ||
             _fixedAmount.has_value());
      mapping.setIntermediateId(id(), solver.makeIntVar(0, 0, 0));
      if (_fixedAmount.has_value()) {
        setViolationVarId(
            solverConstRelation(solver, mapping.intermediateId(id()),
                                *_fixedAmount + _offset, _relType, shouldHold(),
                                true),
            mapping);
      } else {
        setViolationVarId(
            solver.makeIntVar(0, 0, static_cast<Int>(numInputVars())), mapping);
      }
    }
  }
  assert(std::ranges::all_of(outputVarNodeIds(), [&](const VarNodeId vId) {
    return mapping.solverId(vId) != propagation::NULL_ID;
  }));
}

void CountRelNode::registerNode(propagation::SolverBase& solver,
                                SolverMapping& mapping) const {
  if (numInputVars() == 0 && isReified()) {
    assert(violationVarId(mapping).isView());
    return;
  }
  std::vector<propagation::VarViewId> solverVars(numInputVars(),
                                                 propagation::NULL_ID);
  for (size_t i = 0; i < numInputVars(); ++i) {
    solverVars[i] = mapping.solverId(staticInputVarNodeIds()[i]);
  }
  if (_fixedNeedle.has_value()) {
    solver.makeInvariant<propagation::CountConst>(
        solver, mapping.intermediateId(id()), *_fixedNeedle,
        std::move(solverVars), _offset);
  } else {
    assert(_offset == 0);
    solver.makeInvariant<propagation::Count>(
        solver, mapping.intermediateId(id()), mapping.solverId(needle()),
        std::move(solverVars));
  }
  if (!_fixedAmount.has_value()) {
    makeSolverRelation(solver, mapping.intermediateId(id()), _relType,
                       mapping.solverId(amount()), violationVarId(mapping),
                       shouldHold());
  }
}

std::string CountRelNode::dotLangIdentifier() const {
  return "int_lin_eq_node";
}

}  // namespace atlantis::invariantgraph
