#include "atlantis/invariantgraph/violationInvariantNodes/boolLinRelNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/implicitConstraintNodes/boolLinLeImplicitNode.hpp"
#include "atlantis/invariantgraph/implicitConstraintNodes/countImplicitNode.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/invariants/boolLinear.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/utils/overflow.hpp"

namespace atlantis::invariantgraph {

void BoolLinRelNode::updateRelType() {
  if (!isReified() && !shouldHold()) {
    _relType = relationTypeComplement(_relType);
  }
  if (_relType == RelationType::REL_TYPE_GE ||
      _relType == RelationType::REL_TYPE_GT) {
    _rhs = overflow::saturatingSub(0, _rhs);
    for (auto& c : _coeffs) {
      c = overflow::saturatingMul(c, -1);
    }
    _relType = _relType == RelationType::REL_TYPE_GE
                   ? RelationType::REL_TYPE_LE
                   : RelationType::REL_TYPE_LT;
  }
  if (_relType == RelationType::REL_TYPE_LT) {
    _rhs = overflow::saturatingAdd(_rhs, 1);
    _relType = RelationType::REL_TYPE_LT;
  }
  assert(_relType == RelationType::REL_TYPE_EQ ||
         _relType == RelationType::REL_TYPE_NE ||
         _relType == RelationType::REL_TYPE_LE);
}

BoolLinRelNode::BoolLinRelNode(InvariantGraph& graph, std::vector<Int>&& coeffs,
                               std::vector<VarNodeId>&& vars,
                               const RelationType relType, const Int rhs,
                               const VarNodeId reified)
    : ViolationInvariantNode(graph, std::move(vars), reified),
      _relType(relType),
      _coeffs(std::move(coeffs)),
      _rhs(rhs) {}

BoolLinRelNode::BoolLinRelNode(InvariantGraph& graph, std::vector<Int>&& coeffs,
                               std::vector<VarNodeId>&& vars,
                               const RelationType relType, const Int rhs,
                               const bool shouldHold)
    : ViolationInvariantNode(graph, std::move(vars), shouldHold),
      _relType(relType),
      _coeffs(std::move(coeffs)),
      _rhs(rhs) {}

void BoolLinRelNode::init(const InvariantNodeId id) {
  ViolationInvariantNode::init(id);
  updateRelType();
  assert(
      !isReified() ||
      !invariantGraphConst().varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(std::ranges::none_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
}

void BoolLinRelNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  if (isReified()) {
    constraintSolver().bool_lin_reif(
        _coeffs,
        toConstraintVarIds(invariantGraphConst(), staticInputVarNodeIds()),
        _relType, _rhs, reifiedVarNodeConst().constraintVarId());
  } else {
    constraintSolver().bool_lin(
        _coeffs,
        toConstraintVarIds(invariantGraphConst(), staticInputVarNodeIds()),
        _relType, _rhs, shouldHold());
  }
}

void BoolLinRelNode::updateState() {
  ViolationInvariantNode::updateState();
  updateRelType();

  // Remove duplicates:
  for (Int i = 0; i < static_cast<Int>(staticInputVarNodeIds().size()); ++i) {
    for (Int j = static_cast<Int>(staticInputVarNodeIds().size()) - 1; j > i;
         --j) {
      if (staticInputVarNodeIds().at(i) == staticInputVarNodeIds().at(j)) {
        _coeffs.at(i) += _coeffs.at(j);
        _coeffs.erase(_coeffs.begin() + j);
        eraseStaticInputVarNode(j);
      }
    }
  }

  // Remove fixed inputs and inputs with a coefficient of 0 as well as update
  // _offset:
  std::vector<Int> indicesToRemove;
  indicesToRemove.reserve(staticInputVarNodeIds().size());

  for (Int i = 0; i < static_cast<Int>(staticInputVarNodeIds().size()); ++i) {
    const auto& inputNode = staticInputVarNodeConst(i);
    if (inputNode.isFixed() || _coeffs.at(i) == 0) {
      _rhs -= inputNode.inDomain(bool{true}) ? _coeffs.at(i) : 0;
      indicesToRemove.emplace_back(i);
    }
  }

  for (Int i = static_cast<Int>(indicesToRemove.size()) - 1; i >= 0; --i) {
    removeStaticInputVarNode(staticInputVarNodeIds().at(indicesToRemove.at(i)));
    _coeffs.erase(_coeffs.begin() + indicesToRemove.at(i));
  }

  Int lb = 0;
  Int ub = 0;
  for (size_t i = 0; i < staticInputVarNodeIds().size(); ++i) {
    lb += std::min<Int>(0, _coeffs.at(i));
    ub += std::max<Int>(0, _coeffs.at(i));
  }

  if (_relType == RelationType::REL_TYPE_EQ ||
      _relType == RelationType::REL_TYPE_NE) {
    if ((lb == _rhs || ub == _rhs) || (_rhs < lb || ub < _rhs)) {
      assert(!isReified());
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
  } else {
    assert(_relType == RelationType::REL_TYPE_LE);
    if (ub <= _rhs || lb > _rhs) {
      assert(!isReified());
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
  }

  bool sameCoeff = !_coeffs.empty() && std::abs(_coeffs.front()) != 1;
  for (size_t i = 1; sameCoeff && i < _coeffs.size(); ++i) {
    if (std::abs(_coeffs[i]) != std::abs(_coeffs.front())) {
      sameCoeff = false;
    }
  }
  if (sameCoeff) {
    const Int c = std::abs(_coeffs.front());
    if (_rhs % c != 0) {
      assert(!isReified());
      assert(!shouldHold());
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
    for (long& coeff : _coeffs) {
      coeff /= c;
    }
    _rhs /= c;
  }
}

bool BoolLinRelNode::canBeMadeImplicit() const {
  if (state() != InvariantNodeState::ACTIVE || isReified()) {
    return false;
  }
  assert(shouldHold());
  if (_relType == RelationType::REL_TYPE_NE) {
    return false;
  }
  const bool allSourceVars =
      std::ranges::all_of(staticInputVarNodeIds(), [&](const auto& id) {
        return invariantGraphConst().varNodeConst(id).definingNodes().empty();
      });
  if (_relType == RelationType::REL_TYPE_EQ) {
    return allSourceVars &&
           std::ranges::all_of(_coeffs, [&](const Int c) { return c == 1; });
  }
  assert(_relType == RelationType::REL_TYPE_LE);
  return allSourceVars;
}

bool BoolLinRelNode::makeImplicit() {
  if (!canBeMadeImplicit()) {
    return false;
  }
  if (_relType == RelationType::REL_TYPE_EQ) {
    const auto amount = static_cast<size_t>(_rhs);
    invariantGraph().addImplicitConstraintNode(
        std::make_shared<CountImplicitNode>(
            invariantGraph(), std::vector<VarNodeId>(staticInputVarNodeIds()),
            0, amount));
    return true;
  }
  assert(_relType == RelationType::REL_TYPE_LE);
  invariantGraph().addImplicitConstraintNode(
      std::make_shared<BoolLinLeImplicitNode>(
          invariantGraph(), std::move(_coeffs),
          std::vector<VarNodeId>{staticInputVarNodeIds()}, _rhs));
  return true;
}

void BoolLinRelNode::registerOutputVars(propagation::SolverBase& solver,
                                        SolverMapping& mapping) const {
  assert(shouldHold());
  if (violationVarId(mapping) == propagation::NULL_ID) {
    mapping.setIntermediateId(id(), solver.makeIntVar(0, 0, 0));
    setViolationVarId(makeSolverConstIntRelation(solver, mapping.intermediateId(id()),
                                          _relType, _rhs, shouldHold()),
                      mapping);
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void BoolLinRelNode::registerNode(propagation::SolverBase& solver,
                                  SolverMapping& mapping) const {
  assert(violationVarId(mapping) != propagation::NULL_ID);
  assert(violationVarId(mapping).isView());

  assert(mapping.intermediateId(id()) != propagation::NULL_ID);
  assert(mapping.intermediateId(id()).isVar());

  std::vector<propagation::VarViewId> solverVars;
  std::ranges::transform(
      staticInputVarNodeIds(), std::back_inserter(solverVars),
      [&](const VarNodeId varNodeId) {
        assert(mapping.solverId(varNodeId) != propagation::NULL_ID);
        return mapping.solverId(varNodeId);
      });
  solver.makeInvariant<propagation::BoolLinear>(
      solver, mapping.intermediateId(id()), std::vector<Int>(_coeffs),
      std::move(solverVars));
}

std::string BoolLinRelNode::dotLangIdentifier() const {
  return std::string{"bool_lin_"} +
         (_relType == RelationType::REL_TYPE_EQ
              ? "eq"
              : (_relType == RelationType::REL_TYPE_NE ? "ne" : "le"));
}

}  // namespace atlantis::invariantgraph
