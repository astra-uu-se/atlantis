#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolOrNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/views/boolNotNode.hpp"
#include "atlantis/propagation/invariants/boolOr.hpp"
#include "atlantis/propagation/invariants/min.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {

ArrayBoolOrNode::ArrayBoolOrNode(InvariantGraph& graph, VarNodeId a,
                                 VarNodeId b, VarNodeId reified)
    : ViolationInvariantNode(graph, std::vector<VarNodeId>{a, b}, reified) {}

ArrayBoolOrNode::ArrayBoolOrNode(InvariantGraph& graph, VarNodeId a,
                                 VarNodeId b, bool shouldHold)
    : ViolationInvariantNode(graph, std::vector<VarNodeId>{a, b}, shouldHold) {}

ArrayBoolOrNode::ArrayBoolOrNode(InvariantGraph& graph,
                                 std::vector<VarNodeId>&& inputs,
                                 VarNodeId reified)
    : ViolationInvariantNode(graph, std::move(inputs), reified) {}

ArrayBoolOrNode::ArrayBoolOrNode(InvariantGraph& graph,
                                 std::vector<VarNodeId>&& inputs,
                                 bool shouldHold)
    : ViolationInvariantNode(graph, std::move(inputs), shouldHold) {}

void ArrayBoolOrNode::init(InvariantNodeId id) {
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

void ArrayBoolOrNode::updateState() {
  ViolationInvariantNode::updateState();
  if (!isReified() && !shouldHold()) {
    for (const auto& id : staticInputVarNodeIds()) {
      invariantGraph().varNode(id).fixToValue(bool{false});
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }

  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());
  // remove fixed inputs that are false:
  for (const auto& id : staticInputVarNodeIds()) {
    if (invariantGraphConst().varNodeConst(id).isFixed()) {
      if (invariantGraphConst().varNodeConst(id).inDomain(bool{false})) {
        varsToRemove.emplace_back(id);
      } else {
        if (isReified()) {
          fixReified(true);
        } else if (!shouldHold()) {
          throw InconsistencyException(
              "ArrayBoolOrNode::updateState constraint is violated");
        }
        setState(InvariantNodeState::SUBSUMED);
        return;
      }
    }
  }

  for (const auto& id : varsToRemove) {
    removeStaticInputVarNode(id);
  }

  if (staticInputVarNodeIds().empty()) {
    if (isReified()) {
      fixReified(false);
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
  return state() == InvariantNodeState::ACTIVE && isReified() &&
         staticInputVarNodeIds().size() == 1;
}

bool ArrayBoolOrNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (staticInputVarNodeIds().size() == 1 && isReified()) {
    invariantGraph().replaceVarNode(reifiedViolationNodeId(),
                                    staticInputVarNodeIds().front());
  }
  return true;
}

void ArrayBoolOrNode::registerOutputVars() {
  if (staticInputVarNodeIds().size() > 1 && shouldHold() &&
      violationVarId() == propagation::NULL_ID) {
    registerViolation();
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).varId() !=
               propagation::NULL_ID;
      }));
}

void ArrayBoolOrNode::registerNode() {
  if (staticInputVarNodeIds().size() <= 1 || (!isReified() && !shouldHold())) {
    return;
  }
  assert(violationVarId() != propagation::NULL_ID);
  assert(shouldHold());
  assert(violationVarId().isVar());

  std::vector<propagation::VarViewId> solverVars;
  std::ranges::transform(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      std::back_inserter(solverVars),
      [&](const auto& node) { return invariantGraph().varId(node); });

  if (solverVars.size() == 2) {
    solver().makeInvariant<propagation::BoolOr>(
        solver(), violationVarId(), solverVars.front(), solverVars.back());
  } else {
    solver().makeInvariant<propagation::Min>(solver(), violationVarId(),
                                             std::move(solverVars), Int{0});
  }
}

std::string ArrayBoolOrNode::dotLangIdentifier() const {
  return "array_bool_or";
}

}  // namespace atlantis::invariantgraph
