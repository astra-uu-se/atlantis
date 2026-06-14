#include "atlantis/invariantgraph/violationInvariantNodes/globalCardinalityLowUpNode.hpp"

#include <algorithm>
#include <stack>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/propagation/violationInvariants/globalCardinalityLowUp.hpp"

namespace atlantis::invariantgraph {

void initCover(std::vector<Int>& cover, std::vector<Int>& low,
               std::vector<Int>& up) {
  for (Int i = 0; i < static_cast<Int>(cover.size()); i++) {
    for (Int j = static_cast<Int>(cover.size()) - 1; j > i; --j) {
      if (cover[i] == cover[j]) {
        low[i] = std::max(low[i], low[j]);
        up[i] = std::min(up[i], up[j]);
        cover.erase(cover.begin() + j);
        low.erase(low.begin() + j);
        up.erase(up.begin() + j);
      }
    }
  }
}

GlobalCardinalityLowUpNode::GlobalCardinalityLowUpNode(
    InvariantGraph& graph, std::vector<VarNodeId>&& x, std::vector<Int>&& cover,
    std::vector<Int>&& low, std::vector<Int>&& up, const VarNodeId r)
    : ViolationInvariantNode(graph, {}, std::move(x), r),
      _cover(std::move(cover)),
      _low(std::move(low)),
      _up(std::move(up)) {
  initCover(_cover, _low, _up);
}

GlobalCardinalityLowUpNode::GlobalCardinalityLowUpNode(
    InvariantGraph& graph, std::vector<VarNodeId>&& x, std::vector<Int>&& cover,
    std::vector<Int>&& low, std::vector<Int>&& up, const bool shouldHold)
    : ViolationInvariantNode(graph, {}, std::move(x), shouldHold),
      _cover(std::move(cover)),
      _low(std::move(low)),
      _up(std::move(up)) {
  initCover(_cover, _low, _up);
}

void GlobalCardinalityLowUpNode::init(const InvariantNodeId id) {
  ViolationInvariantNode::init(id);
  assert(
      !isReified() ||
      !invariantGraphConst().varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(std::ranges::all_of(
      outputVarNodeIds().begin() + (isReified() ? 1 : 0),
      outputVarNodeIds().end(), [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
  assert(std::ranges::all_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
}

void GlobalCardinalityLowUpNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  if (isReified()) {
    return constraintSolver().fzn_global_cardinality_low_up_reif(
        toConstraintVarIds(invariantGraphConst(), staticInputVarNodeIds()),
        _cover, _low, _up, reifiedVarNodeConst().constraintVarId());
  }
  constraintSolver().fzn_global_cardinality_low_up(
      toConstraintVarIds(invariantGraphConst(), staticInputVarNodeIds()),
      _cover, _low, _up, shouldHold());
}

void GlobalCardinalityLowUpNode::updateState() {
  ViolationInvariantNode::updateState();
  if (isReified()) {
    return;
  }

  for (size_t i = 0; i < _cover.size(); ++i) {
    if (_up[i] < 0 ||
        static_cast<Int>(staticInputVarNodeIds().size()) < _low[i] ||
        _low[i] > _up[i]) {
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
  }

  if (!shouldHold()) {
    const auto bounds =
        gccBounds(invariantGraphConst(), staticInputVarNodeIds(), _cover);
    assert(bounds.size() == _cover.size());
    for (size_t i = 0; i < bounds.size(); i++) {
      if (bounds[i].second < _low[i] || _up[i] < bounds[i].first) {
        setState(InvariantNodeState::SUBSUMED);
      }
    }
    return;
  }

  const auto [varsToRemove, coverIndicesToRemove] = gccUpdateState(
      invariantGraphConst(), staticInputVarNodeIds(), _cover, _low, _up);

  for (Int i = static_cast<Int>(coverIndicesToRemove->size()) - 1; i >= 0;
       --i) {
    _cover.erase(_cover.begin() + i);
    _low.erase(_low.begin() + i);
    _up.erase(_up.begin() + i);
  }

  for (const VarNodeId vId : varsToRemove) {
    for (size_t i = 0; i < _cover.size(); ++i) {
      if (varNodeConst(vId).constDomain()->contains(_cover[i])) {
        assert(varNodeConst(vId).isFixed());
        --_low[i];
        --_up[i];
      }
    }
    removeStaticInputVarNode(vId);
  }

  if (_cover.empty() || staticInputVarNodeIds().empty()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

void GlobalCardinalityLowUpNode::registerOutputVars(
    propagation::SolverBase& solver, SolverMapping& mapping) const {
  if (violationVarId(mapping) == propagation::NULL_ID) {
    if (!shouldHold()) {
      mapping.setIntermediateId(
          id(), solver.makeIntVar(
                    0, 0, static_cast<Int>(staticInputVarNodeIds().size())));
      setViolationVarId(solver.makeIntView<propagation::NotEqualConst>(
                            solver, mapping.intermediateId(id()), 0),
                        mapping);
    } else {
      registerViolation(solver, mapping);
    }
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void GlobalCardinalityLowUpNode::registerNode(propagation::SolverBase& solver,
                                              SolverMapping& mapping) const {
  std::vector<propagation::VarViewId> inputVarIds;
  assert(violationVarId(mapping) != propagation::NULL_ID);
  assert(shouldHold() || mapping.intermediateId(id()) != propagation::NULL_ID);
  assert(shouldHold() ? violationVarId(mapping).isVar()
                      : mapping.intermediateId(id()).isVar());

  std::ranges::transform(staticInputVarNodeIds().begin(),
                         staticInputVarNodeIds().end(),
                         std::back_inserter(inputVarIds),
                         [&](const auto& id) { return mapping.solverId(id); });

  std::vector<Int> low(_cover.size());
  std::vector<Int> up(_cover.size());
  for (size_t i = 0; i < _cover.size(); ++i) {
    low[i] = std::min(std::max(Int{0}, _low[i]),
                      static_cast<Int>(inputVarIds.size()));
    up[i] = std::min(std::max(Int{0}, _up[i]),
                     static_cast<Int>(inputVarIds.size()));
  }

  if (shouldHold()) {
    solver.makeInvariant<propagation::GlobalCardinalityLowUp>(
        solver, violationVarId(mapping), std::move(inputVarIds),
        std::vector<Int>(_cover), std::move(low), std::move(up));
  } else {
    solver.makeInvariant<propagation::GlobalCardinalityLowUp>(
        solver, mapping.intermediateId(id()), std::move(inputVarIds),
        std::vector<Int>(_cover), std::move(low), std::move(up));
  }
}

std::string GlobalCardinalityLowUpNode::dotLangIdentifier() const {
  return "global_cardinality_low_up";
}

}  // namespace atlantis::invariantgraph
