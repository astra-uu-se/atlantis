#include "atlantis/invariantgraph/violationInvariantNodes/globalCardinalityLowUpNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/propagation/violationInvariants/globalCardinalityLowUp.hpp"

namespace atlantis::invariantgraph {

GlobalCardinalityLowUpNode::GlobalCardinalityLowUpNode(
    IInvariantGraph& graph, std::vector<VarNodeId>&& x,
    std::vector<Int>&& cover, std::vector<Int>&& low, std::vector<Int>&& up,
    VarNodeId r)
    : ViolationInvariantNode(graph, {}, std::move(x), r),
      _cover(std::move(cover)),
      _low(std::move(low)),
      _up(std::move(up)) {}

GlobalCardinalityLowUpNode::GlobalCardinalityLowUpNode(
    IInvariantGraph& graph, std::vector<VarNodeId>&& x,
    std::vector<Int>&& cover, std::vector<Int>&& low, std::vector<Int>&& up,
    bool shouldHold)
    : ViolationInvariantNode(graph, {}, std::move(x), shouldHold),
      _cover(std::move(cover)),
      _low(std::move(low)),
      _up(std::move(up)) {}

void GlobalCardinalityLowUpNode::init(InvariantNodeId id) {
  ViolationInvariantNode::init(id);
  assert(
      !isReified() ||
      !invariantGraphConst().varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(
      std::all_of(outputVarNodeIds().begin() + 1, outputVarNodeIds().end(),
                  [&](const VarNodeId vId) {
                    return invariantGraphConst().varNodeConst(vId).isIntVar();
                  }));
  assert(
      std::all_of(staticInputVarNodeIds().begin(),
                  staticInputVarNodeIds().end(), [&](const VarNodeId vId) {
                    return invariantGraphConst().varNodeConst(vId).isIntVar();
                  }));
}

void GlobalCardinalityLowUpNode::registerOutputVars() {
  if (violationVarId() == propagation::NULL_ID) {
    if (!shouldHold()) {
      _intermediate = solver().makeIntVar(
          0, 0, static_cast<Int>(staticInputVarNodeIds().size()));
      setViolationVarId(solver().makeIntView<propagation::NotEqualConst>(
          solver(), _intermediate, 0));
    } else {
      registerViolation();
    }
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId vId) {
                       return invariantGraphConst().varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void GlobalCardinalityLowUpNode::registerNode() {
  std::vector<propagation::VarViewId> inputVarIds;
  assert(violationVarId() != propagation::NULL_ID);
  assert(shouldHold() || _intermediate != propagation::NULL_ID);
  assert(shouldHold() ? violationVarId().isVar() : _intermediate.isVar());

  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(inputVarIds),
                 [&](const auto& id) { return invariantGraph().varId(id); });

  if (shouldHold()) {
    solver().makeInvariant<propagation::GlobalCardinalityLowUp>(
        solver(), violationVarId(), std::move(inputVarIds),
        std::vector<Int>(_cover), std::vector<Int>(_low),
        std::vector<Int>(_up));
  } else {
    solver().makeInvariant<propagation::GlobalCardinalityLowUp>(
        solver(), _intermediate, std::move(inputVarIds),
        std::vector<Int>(_cover), std::vector<Int>(_low),
        std::vector<Int>(_up));
  }
}

}  // namespace atlantis::invariantgraph
