#include "atlantis/invariantgraph/violationInvariantNodes/intLinRelNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/fzn/fzn_all_different_int.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/equalConst.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/utils/overflow.hpp"

namespace atlantis::invariantgraph {

void IntLinRelNode::updateRelType() {
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

IntLinRelNode::IntLinRelNode(InvariantGraph& graph, std::vector<Int>&& coeffs,
                             std::vector<VarNodeId>&& vars,
                             const RelationType relType, const Int rhs,
                             const VarNodeId reified)
    : ViolationInvariantNode(graph, std::move(vars), reified),
      _relType(relType),
      _coeffs(std::move(coeffs)),
      _rhs(rhs) {}

IntLinRelNode::IntLinRelNode(InvariantGraph& graph, std::vector<Int>&& coeffs,
                             std::vector<VarNodeId>&& vars,
                             const RelationType relType, const Int bound,
                             const bool shouldHold)
    : ViolationInvariantNode(graph, std::move(vars), shouldHold),
      _relType(relType),
      _coeffs(std::move(coeffs)),
      _rhs(bound) {}

void IntLinRelNode::init(const InvariantNodeId id) {
  ViolationInvariantNode::init(id);
  updateRelType();
  assert(!isReified() || !reifiedVarNodeConst().isIntVar());
  assert(std::ranges::all_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const VarNodeId vId) { return varNodeConst(vId).isIntVar(); }));
}

void IntLinRelNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  if (isReified()) {
    return constraintSolver().int_lin_reif(
        _coeffs,
        toConstraintVarIds(invariantGraphConst(), staticInputVarNodeIds()),
        _relType, _rhs, reifiedVarNodeConst().constraintVarId());
  }
  constraintSolver().int_lin(
      _coeffs,
      toConstraintVarIds(invariantGraphConst(), staticInputVarNodeIds()),
      _relType, _rhs, shouldHold());
}

void IntLinRelNode::updateState() {
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

  std::vector<Int> indicesToRemove;
  indicesToRemove.reserve(staticInputVarNodeIds().size());

  for (Int i = 0; i < static_cast<Int>(staticInputVarNodeIds().size()); ++i) {
    const auto& inputNode =
        invariantGraphConst().varNodeConst(staticInputVarNodeIds().at(i));
    if (inputNode.isFixed() || _coeffs.at(i) == 0) {
      _rhs -= _coeffs.at(i) * inputNode.lowerBound();
      indicesToRemove.emplace_back(i);
    }
  }

  for (Int i = static_cast<Int>(indicesToRemove.size()) - 1; i >= 0; --i) {
    removeStaticInputVarNode(staticInputVarNodeIds().at(indicesToRemove.at(i)));
    _coeffs.erase(_coeffs.begin() + indicesToRemove.at(i));
  }

  if (staticInputVarNodeIds().empty()) {
    setState(InvariantNodeState::SUBSUMED);
    return;
  }

  Int lb = 0;
  Int ub = 0;
  for (size_t i = 0; i < staticInputVarNodeIds().size(); ++i) {
    const Int varLb = staticInputVarNodeConst(i).lowerBound();
    const Int varUb = staticInputVarNodeConst(i).upperBound();
    const Int prod1 = overflow::saturatingMul(_coeffs[i], varLb);
    const Int prod2 = overflow::saturatingMul(_coeffs[i], varUb);
    lb = overflow::saturatingAdd(lb, std::min(prod1, prod2));
    ub = overflow::saturatingAdd(ub, std::max(prod1, prod2));
  }

  if (_relType == RelationType::REL_TYPE_EQ ||
      _relType == RelationType::REL_TYPE_NE) {
    if ((lb == ub && lb == _rhs) || _rhs < lb || ub < _rhs) {
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

void IntLinRelNode::registerOutputVars(propagation::SolverBase& solver,
                                       SolverMapping& mapping) const {
  if (violationVarId(mapping) == propagation::NULL_ID) {
    mapping.setIntermediateId(id(), solver.makeIntVar(0, 0, 0));
    if (shouldHold()) {
      setViolationVarId(solver.makeIntView<propagation::EqualConst>(
                            solver, mapping.intermediateId(id()), _rhs),
                        mapping);
    } else {
      assert(!isReified());
      setViolationVarId(solver.makeIntView<propagation::NotEqualConst>(
                            solver, mapping.intermediateId(id()), _rhs),
                        mapping);
    }
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void IntLinRelNode::registerNode(propagation::SolverBase& solver,
                                 SolverMapping& mapping) const {
  assert(violationVarId(mapping) != propagation::NULL_ID);
  assert(violationVarId(mapping).isView());

  assert(mapping.intermediateId(id()) != propagation::NULL_ID);
  assert(mapping.intermediateId(id()).isVar());

  std::vector<propagation::VarViewId> solverVars;
  solverVars.reserve(staticInputVarNodeIds().size());
  std::ranges::transform(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      std::back_inserter(solverVars), [&](const VarNodeId varNodeId) {
        assert(mapping.solverId(varNodeId) != propagation::NULL_ID);
        return mapping.solverId(varNodeId);
      });
  solver.makeInvariant<propagation::Linear>(
      solver, mapping.intermediateId(id()), std::vector<Int>(_coeffs),
      std::move(solverVars));
}

const std::vector<Int>& IntLinRelNode::coeffs() const { return _coeffs; }

std::string IntLinRelNode::dotLangIdentifier() const {
  return "int_lin_eq_node";
}

}  // namespace atlantis::invariantgraph
