#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolAndNode.hpp"

#include "../parseHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/views/boolNotNode.hpp"
#include "atlantis/propagation/invariants/boolAnd.hpp"
#include "atlantis/propagation/invariants/max.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {

ArrayBoolAndNode::ArrayBoolAndNode(IInvariantGraph& graph, VarNodeId a,
                                   VarNodeId b, VarNodeId output)
    : ViolationInvariantNode(graph, std::vector<VarNodeId>{a, b}, output) {}

ArrayBoolAndNode::ArrayBoolAndNode(IInvariantGraph& graph, VarNodeId a,
                                   VarNodeId b, bool shouldHold)
    : ViolationInvariantNode(graph, std::vector<VarNodeId>{a, b}, shouldHold) {}

ArrayBoolAndNode::ArrayBoolAndNode(IInvariantGraph& graph,
                                   std::vector<VarNodeId>&& as,
                                   VarNodeId output)
    : ViolationInvariantNode(graph, std::move(as), output) {}

ArrayBoolAndNode::ArrayBoolAndNode(IInvariantGraph& graph,
                                   std::vector<VarNodeId>&& as, bool shouldHold)
    : ViolationInvariantNode(graph, std::move(as), shouldHold) {}

void ArrayBoolAndNode::init(InvariantNodeId id) {
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

void ArrayBoolAndNode::updateState() {
  ViolationInvariantNode::updateState();
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());
  // remove fixed inputs that are true:
  for (const auto& id : staticInputVarNodeIds()) {
    if (invariantGraphConst().varNodeConst(id).isFixed()) {
      if (invariantGraphConst().varNodeConst(id).inDomain(bool{true})) {
        varsToRemove.emplace_back(id);
      } else if (isReified()) {
        fixReified(false);
      } else if (shouldHold()) {
        throw InconsistencyException(
            "ArrayBoolAndNode::updateState constraint is violated");
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
    } else if (!shouldHold()) {
      throw InconsistencyException(
          "ArrayBoolAndNode::updateState constraint is violated");
    }
    setState(InvariantNodeState::SUBSUMED);
  } else if (staticInputVarNodeIds().size() == 1 && !isReified()) {
    auto& inputNode = invariantGraph().varNode(staticInputVarNodeIds().front());
    inputNode.fixToValue(shouldHold());
    removeStaticInputVarNode(inputNode.varNodeId());
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool ArrayBoolAndNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         staticInputVarNodeIds().size() <= 1;
}

bool ArrayBoolAndNode::replace() {
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

void ArrayBoolAndNode::registerOutputVars() {
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

void ArrayBoolAndNode::registerNode() {
  if (staticInputVarNodeIds().size() <= 1) {
    return;
  }
  assert(violationVarId() != propagation::NULL_ID);
  assert(shouldHold() || _intermediate != propagation::NULL_ID);
  assert(shouldHold() ? violationVarId().isVar() : _intermediate.isVar());

  std::vector<propagation::VarViewId> solverVars;
  solverVars.reserve(staticInputVarNodeIds().size());
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars), [&](const auto& node) {
                   return invariantGraphConst().varId(node);
                 });
  if (solverVars.size() == 2) {
    solver().makeInvariant<propagation::BoolAnd>(
        solver(), !shouldHold() ? _intermediate : violationVarId(),
        solverVars.front(), solverVars.back());
  } else {
    solver().makeInvariant<propagation::Max>(
        solver(), !shouldHold() ? _intermediate : violationVarId(),
        std::move(solverVars));
  }
}

}  // namespace atlantis::invariantgraph
