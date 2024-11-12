#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolOrNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/views/boolNotNode.hpp"
#include "atlantis/propagation/invariants/boolOr.hpp"
#include "atlantis/propagation/invariants/min.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {

ArrayBoolOrNode::ArrayBoolOrNode(IInvariantGraph& graph, VarNodeId a,
                                 VarNodeId b, VarNodeId reified)
    : ViolationInvariantNode(graph, std::vector<VarNodeId>{a, b}, reified) {}

ArrayBoolOrNode::ArrayBoolOrNode(IInvariantGraph& graph, VarNodeId a,
                                 VarNodeId b, bool shouldHold)
    : ViolationInvariantNode(graph, std::vector<VarNodeId>{a, b}, shouldHold) {}

ArrayBoolOrNode::ArrayBoolOrNode(IInvariantGraph& graph,
                                 std::vector<VarNodeId>&& inputs,
                                 VarNodeId reified)
    : ViolationInvariantNode(graph, std::move(inputs), reified) {}

ArrayBoolOrNode::ArrayBoolOrNode(IInvariantGraph& graph,
                                 std::vector<VarNodeId>&& inputs,
                                 bool shouldHold)
    : ViolationInvariantNode(graph, std::move(inputs), shouldHold) {}

void ArrayBoolOrNode::init(InvariantNodeId id) {
  ViolationInvariantNode::init(id);
  assert(
      !isReified() ||
      !invariantGraphConst().varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(
      std::none_of(staticInputVarNodeIds().begin(),
                   staticInputVarNodeIds().end(), [&](const VarNodeId vId) {
                     return invariantGraphConst().varNodeConst(vId).isIntVar();
                   }));
}

void ArrayBoolOrNode::updateState() {
  ViolationInvariantNode::updateState();
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());
  // remove fixed inputs that are false:
  for (const auto& id : staticInputVarNodeIds()) {
    if (invariantGraphConst().varNodeConst(id).isFixed()) {
      if (invariantGraphConst().varNodeConst(id).inDomain(bool{false})) {
        varsToRemove.emplace_back(id);
      } else if (isReified()) {
        fixReified(true);
      } else if (!shouldHold()) {
        throw InconsistencyException(
            "ArrayBoolOrNode::updateState constraint is violated");
      } else {
        setState(InvariantNodeState::SUBSUMED);
        return;
      }
    }
  }

  for (const auto& id : varsToRemove) {
    removeStaticInputVarNode(id);
  }

  if (staticInputVarNodeIds().size() == 0) {
    if (isReified()) {
      fixReified(true);
    } else if (shouldHold()) {
      throw InconsistencyException(
          "ArrayBoolOrNode::updateState constraint is violated");
    }
    setState(InvariantNodeState::SUBSUMED);
  } else if (staticInputVarNodeIds().size() == 1 && !isReified()) {
    auto& inputNode = invariantGraph().varNode(staticInputVarNodeIds().front());
    inputNode.fixToValue(shouldHold());
    removeStaticInputVarNode(inputNode.varNodeId());
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool ArrayBoolOrNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         staticInputVarNodeIds().size() <= 1;
}

bool ArrayBoolOrNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (staticInputVarNodeIds().size() == 1) {
    if (isReified()) {
      invariantGraph().replaceVarNode(reifiedViolationNodeId(),
                                      staticInputVarNodeIds().front());
    }
  }
  return true;
}

void ArrayBoolOrNode::registerOutputVars() {
  if (staticInputVarNodeIds().size() > 1 &&
      violationVarId() == propagation::NULL_ID) {
    if (shouldHold()) {
      registerViolation();
    } else {
      assert(!isReified());
      _intermediate = solver().makeIntVar(0, 0, 0);
      setViolationVarId(solver().makeIntView<propagation::NotEqualConst>(
          solver(), _intermediate, 0));
    }
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId vId) {
                       return invariantGraphConst().varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void ArrayBoolOrNode::registerNode() {
  if (staticInputVarNodeIds().size() <= 1) {
    return;
  }
  assert(violationVarId() != propagation::NULL_ID);
  assert(shouldHold() || _intermediate != propagation::NULL_ID);
  assert(shouldHold() ? violationVarId().isVar() : _intermediate.isVar());

  std::vector<propagation::VarViewId> solverVars;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars), [&](const auto& node) {
                   return invariantGraph().varId(node);
                 });

  if (solverVars.size() == 2) {
    solver().makeInvariant<propagation::BoolOr>(
        solver(), !shouldHold() ? _intermediate : violationVarId(),
        solverVars.front(), solverVars.back());
  } else {
    solver().makeInvariant<propagation::Min>(
        solver(), !shouldHold() ? _intermediate : violationVarId(),
        std::move(solverVars), Int{0});
  }
}

}  // namespace atlantis::invariantgraph
