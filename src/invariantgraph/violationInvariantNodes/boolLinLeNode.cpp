#include "atlantis/invariantgraph/violationInvariantNodes/boolLinLeNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/invariants/boolLinear.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/greaterEqualConst.hpp"
#include "atlantis/propagation/views/lessEqualConst.hpp"

namespace atlantis::invariantgraph {

BoolLinLeNode::BoolLinLeNode(InvariantGraph& graph, std::vector<Int>&& coeffs,
                             std::vector<VarNodeId>&& vars, const Int bound,
                             const VarNodeId reified)
    : ViolationInvariantNode(graph, std::move(vars), reified),
      _coeffs(std::move(coeffs)),
      _bound(bound) {}

BoolLinLeNode::BoolLinLeNode(InvariantGraph& graph, std::vector<Int>&& coeffs,
                             std::vector<VarNodeId>&& vars, const Int bound,
                             const bool shouldHold)
    : ViolationInvariantNode(graph, std::move(vars), shouldHold),
      _coeffs(std::move(coeffs)),
      _bound(bound) {}

void BoolLinLeNode::init(const InvariantNodeId id) {
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

void BoolLinLeNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  if (isReified()) {
    constraintSolver().bool_lin_le_reif(
        _coeffs,
        toConstraintVarIds(invariantGraphConst(), staticInputVarNodeIds()),
        _bound, reifiedVarNodeConst().constraintVarId());
  } else {
    constraintSolver().bool_lin_le(
        _coeffs,
        toConstraintVarIds(invariantGraphConst(), staticInputVarNodeIds()),
        _bound, shouldHold());
  }
}

void BoolLinLeNode::updateState() {
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
  // _bound:
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

  if (ub <= _bound || _bound < lb) {
    assert(!isReified());
    assert((ub <= _bound) == shouldHold());
    setState(InvariantNodeState::SUBSUMED);
  }
}

void BoolLinLeNode::registerOutputVars(propagation::SolverBase& solver,
                                       SolverMapping& mapping) const {
  if (violationVarId(mapping) == propagation::NULL_ID) {
    mapping.setIntermediateId(id(), solver.makeIntVar(0, 0, 0));
    if (shouldHold()) {
      setViolationVarId(solver.makeIntView<propagation::LessEqualConst>(
                            solver, mapping.intermediateId(id()), _bound),
                        mapping);
    } else {
      assert(!isReified());
      setViolationVarId(solver.makeIntView<propagation::GreaterEqualConst>(
                            solver, mapping.intermediateId(id()), _bound + 1),
                        mapping);
    }
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void BoolLinLeNode::registerNode(propagation::SolverBase& solver,
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

const std::vector<Int>& BoolLinLeNode::coeffs() const { return _coeffs; }

std::string BoolLinLeNode::dotLangIdentifier() const { return "bool_lin_le"; }

}  // namespace atlantis::invariantgraph
