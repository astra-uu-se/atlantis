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
  // GCC can define the same output multiple times. Therefore, split all outputs
  // that are defined multiple times:
  postAllEqualOnReplacedVars(invariantGraph(), splitOutputVarNodes());

  ViolationInvariantNode::updateState();
  if (!isReified() || !shouldHold()) {
    return;
  }

  const auto [varsToRemove, coverIndicesToRemove] = gccUpdateState(
      invariantGraphConst(), staticInputVarNodeIds(), _cover, _low, _up);

  for (const VarNodeId vId : varsToRemove) {
    removeStaticInputVarNode(vId);
  }

  const Int outputIndexOffset =
      reifiedViolationNodeId() == NULL_NODE_ID ? 0 : 1;
  assert(outputIndexOffset == 0 ||
         outputVarNodeIds().front() == reifiedViolationNodeId());
  for (Int i = static_cast<Int>(coverIndicesToRemove->size()) - 1; i >= 0;
       --i) {
    _cover.erase(_cover.begin() + i);
    _low.erase(_low.begin() + i);
    _up.erase(_up.begin() + i);
    removeOutputAtIndex(i + outputIndexOffset);
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

  if (shouldHold()) {
    solver.makeInvariant<propagation::GlobalCardinalityLowUp>(
        solver, violationVarId(mapping), std::move(inputVarIds),
        std::vector<Int>(_cover), std::vector<Int>(_low),
        std::vector<Int>(_up));
  } else {
    solver.makeInvariant<propagation::GlobalCardinalityLowUp>(
        solver, mapping.intermediateId(id()), std::move(inputVarIds),
        std::vector<Int>(_cover), std::vector<Int>(_low),
        std::vector<Int>(_up));
  }
}

std::string GlobalCardinalityLowUpNode::dotLangIdentifier() const {
  return "global_cardinality_low_up";
}

}  // namespace atlantis::invariantgraph
