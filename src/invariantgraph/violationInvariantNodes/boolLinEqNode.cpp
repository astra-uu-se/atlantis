#include "atlantis/invariantgraph/violationInvariantNodes/boolLinEqNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/implicitConstraintNodes/countImplicitNode.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/invariants/boolLinear.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/equalConst.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/search/neighborhoods/countNeighborhood.hpp"

namespace atlantis::invariantgraph {

BoolLinEqNode::BoolLinEqNode(InvariantGraph& graph, std::vector<Int>&& coeffs,
                             std::vector<VarNodeId>&& vars, const Int bound,
                             const VarNodeId reified)
    : ViolationInvariantNode(graph, std::move(vars), reified),
      _coeffs(std::move(coeffs)),
      _bound(bound) {}

BoolLinEqNode::BoolLinEqNode(InvariantGraph& graph, std::vector<Int>&& coeffs,
                             std::vector<VarNodeId>&& vars, const Int bound,
                             const bool shouldHold)
    : ViolationInvariantNode(graph, std::move(vars), shouldHold),
      _coeffs(std::move(coeffs)),
      _bound(bound) {}

void BoolLinEqNode::init(const InvariantNodeId id) {
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

void BoolLinEqNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  std::vector<ConstraintVarId> inputs(staticInputVarNodeIds().size(),
                                      ConstraintVarId{NULL_NODE_ID});
  for (size_t i = 0; i < staticInputVarNodeIds().size(); i++) {
    inputs[i] = staticInputVarNode(i).constraintVarId();
  }
  if (isReified()) {
    constraintSolver().bool_lin_eq_reif(
        _coeffs, inputs, _bound, reifiedVarNodeConst().constraintVarId());
  } else {
    constraintSolver().bool_lin_eq(_coeffs, inputs, _bound, shouldHold());
  }
}

void BoolLinEqNode::updateState() {
  ViolationInvariantNode::updateState();
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
      _bound -= inputNode.inDomain(bool{true}) ? _coeffs.at(i) : 0;
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

  if (lb == _bound || ub == _bound) {
    assert(!isReified());
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (_bound < lb || ub < _bound) {
    assert(!isReified());
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  bool sameCoeff = !_coeffs.empty() && std::abs(_coeffs.front()) != 1;
  for (size_t i = 1; sameCoeff && i < _coeffs.size(); ++i) {
    if (std::abs(_coeffs[i]) != std::abs(_coeffs.front())) {
      sameCoeff = false;
    }
  }
  if (sameCoeff) {
    const Int c = std::abs(_coeffs.front());
    if (_bound % c != 0) {
      assert(!isReified());
      assert(!shouldHold());
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
    for (long& coeff : _coeffs) {
      coeff /= c;
    }
    _bound /= c;
  }
}

bool BoolLinEqNode::canBeMadeImplicit() const {
  return state() == InvariantNodeState::ACTIVE && !isReified() &&
         shouldHold() &&
         std::ranges::all_of(staticInputVarNodeIds(),
                             [&](const auto& id) {
                               return invariantGraphConst()
                                   .varNodeConst(id)
                                   .definingNodes()
                                   .empty();
                             }) &&
         std::ranges::all_of(_coeffs, [&](const Int c) { return c == 1; });
}

bool BoolLinEqNode::makeImplicit() {
  if (!canBeMadeImplicit()) {
    return false;
  }
  const auto amount = static_cast<size_t>(_bound);
  invariantGraph().addImplicitConstraintNode(
      std::make_shared<CountImplicitNode>(
          invariantGraph(), std::vector<VarNodeId>(staticInputVarNodeIds()), 0,
          amount));
  return true;
}

void BoolLinEqNode::registerOutputVars(propagation::SolverBase& solver,
                                       SolverMapping& mapping) const {
  if (violationVarId(mapping) == propagation::NULL_ID) {
    mapping.setIntermediateId(id(), solver.makeIntVar(0, 0, 0));
    if (shouldHold()) {
      setViolationVarId(solver.makeIntView<propagation::EqualConst>(
                            solver, mapping.intermediateId(id()), _bound),
                        mapping);
    } else {
      assert(!isReified());
      setViolationVarId(solver.makeIntView<propagation::NotEqualConst>(
                            solver, mapping.intermediateId(id()), _bound),
                        mapping);
    }
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void BoolLinEqNode::registerNode(propagation::SolverBase& solver,
                                 SolverMapping& mapping) const {
  assert(violationVarId(mapping) != propagation::NULL_ID);
  assert(violationVarId(mapping).isView());

  assert(mapping.intermediateId(id()) != propagation::NULL_ID);
  assert(mapping.intermediateId(id()).isVar());

  std::vector<propagation::VarViewId> solverVars;
  std::ranges::transform(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      std::back_inserter(solverVars), [&](const VarNodeId varNodeId) {
        assert(mapping.solverId(varNodeId) != propagation::NULL_ID);
        return mapping.solverId(varNodeId);
      });
  solver.makeInvariant<propagation::BoolLinear>(
      solver, mapping.intermediateId(id()), std::vector<Int>(_coeffs),
      std::move(solverVars));
}

const std::vector<Int>& BoolLinEqNode::coeffs() const { return _coeffs; }

std::string BoolLinEqNode::dotLangIdentifier() const { return "bool_lin_eq"; }

}  // namespace atlantis::invariantgraph
