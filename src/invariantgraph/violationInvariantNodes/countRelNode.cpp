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

VarNodeId CountRelNode::bound() const {
  return _fixedBound.has_value() ? NULL_NODE_ID
                                 : staticInputVarNodeIds()[boundIndex()];
  ;
}

size_t CountRelNode::boundIndex() const {
  return numInputVars() + (_fixedNeedle.has_value() ? 0 : 1);
}

size_t CountRelNode::numInputVars() const {
  return staticInputVarNodeIds().size() - (_fixedNeedle.has_value() ? 0 : 1) -
         (_fixedBound.has_value() ? 0 : 1);
}

CountRelNode::CountRelNode(InvariantGraph& graph, const Int amount,
                           const RelationType relationType,
                           std::vector<VarNodeId>&& vars, const Int needle,
                           const bool shouldHold)
    : ViolationInvariantNode(graph, {}, std::move(vars), shouldHold),
      _fixedNeedle(needle),
      _fixedBound(amount),
      _relType(relationType) {}

CountRelNode::CountRelNode(InvariantGraph& graph, const VarNodeId amount,
                           const RelationType relationType,
                           std::vector<VarNodeId>&& vars, const Int needle,
                           const bool shouldHold)
    : ViolationInvariantNode(graph, {}, append(std::move(vars), amount),
                             shouldHold),
      _fixedNeedle(needle),
      _fixedBound(std::nullopt),
      _relType(relationType) {}

CountRelNode::CountRelNode(InvariantGraph& graph, const Int amount,
                           const RelationType relationType,
                           std::vector<VarNodeId>&& vars,
                           const VarNodeId needle, const bool shouldHold)
    : ViolationInvariantNode(graph, {}, append(std::move(vars), needle),
                             shouldHold),
      _fixedNeedle(std::nullopt),
      _fixedBound(amount),
      _relType(relationType) {}

CountRelNode::CountRelNode(InvariantGraph& graph, const VarNodeId amount,
                           const RelationType relationType,
                           std::vector<VarNodeId>&& vars,
                           const VarNodeId needle, const bool shouldHold)
    : ViolationInvariantNode(graph, {},
                             append(append(std::move(vars), needle), amount),
                             shouldHold),
      _fixedNeedle(std::nullopt),
      _fixedBound(std::nullopt),
      _relType(relationType) {}

CountRelNode::CountRelNode(InvariantGraph& graph, const Int amount,
                           const RelationType relationType,
                           std::vector<VarNodeId>&& vars, const Int needle,
                           const VarNodeId reified)
    : ViolationInvariantNode(graph, {}, std::move(vars), reified),
      _fixedNeedle(needle),
      _fixedBound(amount),
      _relType(relationType) {}

CountRelNode::CountRelNode(InvariantGraph& graph, const VarNodeId amount,
                           const RelationType relationType,
                           std::vector<VarNodeId>&& vars, const Int needle,
                           const VarNodeId reified)
    : ViolationInvariantNode(graph, {}, append(std::move(vars), amount),
                             reified),
      _fixedNeedle(needle),
      _fixedBound(std::nullopt),
      _relType(relationType) {}

CountRelNode::CountRelNode(InvariantGraph& graph, const Int amount,
                           const RelationType relationType,
                           std::vector<VarNodeId>&& vars,
                           const VarNodeId needle, const VarNodeId reified)
    : ViolationInvariantNode(graph, {}, append(std::move(vars), needle),
                             reified),
      _fixedNeedle(std::nullopt),
      _fixedBound(amount),
      _relType(relationType) {}

CountRelNode::CountRelNode(InvariantGraph& graph, const VarNodeId amount,
                           const RelationType relationType,
                           std::vector<VarNodeId>&& vars,
                           const VarNodeId needle, const VarNodeId reified)
    : ViolationInvariantNode(
          graph, {}, append(append(std::move(vars), needle), amount), reified),
      _fixedNeedle(std::nullopt),
      _fixedBound(std::nullopt),
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
  assert(_boundOffset == 0);
  if (isReified()) {
    if (_fixedNeedle.has_value()) {
      if (_fixedBound.has_value()) {
        return constraintSolver().fzn_count_reif(
            *_fixedBound, _relType, inputs, *_fixedNeedle,
            reifiedVarNodeConst().constraintVarId());
      }
      return constraintSolver().fzn_count_reif(
          varNodeConst(bound()).constraintVarId(), _relType, inputs,
          *_fixedNeedle, reifiedVarNodeConst().constraintVarId());
    }
    if (_fixedBound.has_value()) {
      return constraintSolver().fzn_count_reif(
          *_fixedBound, _relType, inputs,
          varNodeConst(needle()).constraintVarId(),
          reifiedVarNodeConst().constraintVarId());
    }
    return constraintSolver().fzn_count_reif(
        varNodeConst(bound()).constraintVarId(), _relType, inputs,
        varNodeConst(needle()).constraintVarId(),
        reifiedVarNodeConst().constraintVarId());
  }
  if (_fixedNeedle.has_value()) {
    if (_fixedBound.has_value()) {
      return constraintSolver().fzn_count(*_fixedBound, _relType, inputs,
                                          *_fixedNeedle, shouldHold());
    }
    return constraintSolver().fzn_count(varNodeConst(bound()).constraintVarId(),
                                        _relType, inputs, *_fixedNeedle,
                                        shouldHold());
  }
  if (_fixedBound.has_value()) {
    return constraintSolver().fzn_count(
        *_fixedBound, _relType, inputs,
        varNodeConst(needle()).constraintVarId(), shouldHold());
  }
  return constraintSolver().fzn_count(
      varNodeConst(bound()).constraintVarId(), _relType, inputs,
      varNodeConst(needle()).constraintVarId(), shouldHold());
}

void CountRelNode::updateState() {
  ViolationInvariantNode::updateState();
  // update fixed needle and amount
  if (!_fixedNeedle.has_value() && varNodeConst(needle()).isFixed()) {
    // updating _fixedNeedle modifies indices
    const Int n = varNodeConst(needle()).lowerBound();
    removeStaticInputAtIndex(needleIndex());
    _fixedNeedle = n;
  }
  if (!_fixedBound.has_value() && varNodeConst(bound()).isFixed()) {
    // updating _fixedAmount modifies indices
    const Int a = varNodeConst(bound()).lowerBound();
    removeStaticInputAtIndex(boundIndex());
    _fixedBound = a;
  }
  if (isReified()) {
    return;
  }
  if (!isReified() && !shouldHold()) {
    setShouldHold(true);
    _relType = relationTypeComplement(_relType);
  }
  std::vector<Int> indicesToRemove;
  indicesToRemove.reserve(numInputVars());
  for (Int i = static_cast<Int>(numInputVars()) - 1; i >= 0; --i) {
    if (_fixedNeedle.has_value() && staticInputVarNodeConst(i).isFixed()) {
      _boundOffset +=
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

  const Int needleLb = _fixedNeedle.has_value()
                           ? *_fixedNeedle
                           : varNodeConst(needle()).lowerBound();
  const Int needleUb = _fixedNeedle.has_value()
                           ? *_fixedNeedle
                           : varNodeConst(needle()).upperBound();

  std::vector<std::pair<Int, Int>> domIntervals(numInputVars());
  for (size_t i = 0; i < numInputVars(); ++i) {
    domIntervals[i] = std::pair<Int, Int>{
        std::max(needleLb,
                 std::min(needleUb, staticInputVarNodeConst(i).lowerBound())),
        std::max(needleLb,
                 std::min(needleUb, staticInputVarNodeConst(i).upperBound()))};
  }

  const Int actualLb = _boundOffset;
  const Int actualUb = _boundOffset + maxOverlaps(domIntervals);

  const Int boundLb = _fixedBound.has_value()
                          ? *_fixedBound
                          : varNodeConst(bound()).lowerBound();
  const Int boundUb = _fixedBound.has_value()
                          ? *_fixedBound
                          : varNodeConst(bound()).upperBound();

  assert(actualLb < actualUb || numInputVars() == 0);
  if (_relType == RelationType::REL_TYPE_EQ) {
    if (boundLb == boundUb && actualLb == actualUb && boundLb == actualUb) {
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
  } else if (_relType == RelationType::REL_TYPE_NE) {
    if (actualUb < boundLb || boundUb < actualLb) {
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
  } else if (_relType == RelationType::REL_TYPE_GE) {
    if (boundLb >= actualUb) {
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
  } else if (_relType == RelationType::REL_TYPE_GT) {
    if (boundLb > actualUb) {
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
  } else if (_relType == RelationType::REL_TYPE_LT) {
    if (boundUb < actualLb) {
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
  } else if (_relType == RelationType::REL_TYPE_LE) {
    if (boundUb <= actualLb) {
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
  }
}

bool CountRelNode::canBeReplaced() const {
  if (state() != InvariantNodeState::ACTIVE) {
    return false;
  }
  return !isReified() && !_fixedBound.has_value() &&
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
        invariantGraph(), bound(), std::move(varNodeIds), *_fixedNeedle));
  } else {
    invariantGraph().addInvariantNode(std::make_shared<CountNode>(
        invariantGraph(), bound(), std::move(varNodeIds), needle()));
  }
  return true;
}

void CountRelNode::registerOutputVars(propagation::SolverBase& solver,
                                      SolverMapping& mapping) const {
  if (violationVarId(mapping) == propagation::NULL_ID) {
    if (numInputVars() == 0 && isReified()) {
      assert(!_fixedBound.has_value());
      setViolationVarId(
          makeSolverConstIntRelation(solver, mapping.solverId(bound()),
                                     _relType, _boundOffset, shouldHold()),
          mapping);
    } else {
      assert(isReified() ||
             (shouldHold() ? _relType : relationTypeComplement(_relType)) !=
                 RelationType::REL_TYPE_EQ ||
             _fixedBound.has_value());
      mapping.setIntermediateId(id(), solver.makeIntVar(0, 0, 0));
      if (_fixedBound.has_value()) {
        setViolationVarId(makeSolverConstIntRelation(
                              solver, mapping.intermediateId(id()), _relType,
                              *_fixedBound + _boundOffset, shouldHold(), true),
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
        std::move(solverVars), _boundOffset);
  } else {
    assert(_boundOffset == 0);
    solver.makeInvariant<propagation::Count>(
        solver, mapping.intermediateId(id()), mapping.solverId(needle()),
        std::move(solverVars));
  }
  if (!_fixedBound.has_value()) {
    makeSolverIntRelation(solver, mapping.solverId(bound()), _relType,
                          mapping.intermediateId(id()), violationVarId(mapping),
                          shouldHold());
  }
}

std::string CountRelNode::dotLangIdentifier() const {
  return "int_lin_eq_node";
}

}  // namespace atlantis::invariantgraph
