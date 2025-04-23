#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolXorNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/views/boolNotNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolAllEqualNode.hpp"
#include "atlantis/propagation/invariants/boolLinear.hpp"
#include "atlantis/propagation/invariants/boolXor.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/equalConst.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {

ArrayBoolXorNode::ArrayBoolXorNode(InvariantGraph& graph, VarNodeId a,
                                   VarNodeId b, VarNodeId reified)
    : ArrayBoolXorNode(graph, std::vector<VarNodeId>{a, b}, reified) {}

ArrayBoolXorNode::ArrayBoolXorNode(InvariantGraph& graph, VarNodeId a,
                                   VarNodeId b, bool shouldHold)
    : ArrayBoolXorNode(graph, std::vector<VarNodeId>{a, b}, shouldHold) {}

ArrayBoolXorNode::ArrayBoolXorNode(InvariantGraph& graph,
                                   std::vector<VarNodeId>&& inputs,
                                   VarNodeId reified)
    : ViolationInvariantNode(graph, std::move(inputs), reified) {}

ArrayBoolXorNode::ArrayBoolXorNode(InvariantGraph& graph,
                                   std::vector<VarNodeId>&& inputs,
                                   bool shouldHold)
    : ViolationInvariantNode(graph, std::move(inputs), shouldHold) {}

void ArrayBoolXorNode::init(InvariantNodeId id) {
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

void ArrayBoolXorNode::updateState() {
  ViolationInvariantNode::updateState();
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());
  // keep track of index of fixed input that is true:
  size_t trueIndex = staticInputVarNodeIds().size();
  // remove fixed inputs that are false:
  for (size_t i = 0; i < staticInputVarNodeIds().size(); ++i) {
    VarNode& iNode = invariantGraph().varNode(staticInputVarNodeIds().at(i));
    if (!iNode.isFixed()) {
      continue;
    }
    if (iNode.inDomain(bool{false})) {
      varsToRemove.emplace_back(staticInputVarNodeIds().at(i));
    } else if (trueIndex >= staticInputVarNodeIds().size()) {
      trueIndex = i;
    } else {
      // Two or more inputs are fixed to true
      if (isReified()) {
        // this violation invariant is no longer reified:
        fixReified(false);
        assert(!isReified() && !shouldHold());
      } else if (shouldHold()) {
        throw InconsistencyException(
            "ArrayBoolOrNode::updateState constraint is violated");
      }
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
  }
  if (trueIndex < staticInputVarNodeIds().size() && !isReified() &&
      shouldHold()) {
    // fix all inputs beside the true input to false:
    for (size_t i = 0; i < staticInputVarNodeIds().size(); ++i) {
      if (i == trueIndex) {
        continue;
      }
      invariantGraph()
          .varNode(staticInputVarNodeIds().at(i))
          .fixToValue(bool{false});
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }

  for (const auto& id : varsToRemove) {
    removeStaticInputVarNode(id);
  }

  if (staticInputVarNodeIds().empty()) {
    // array_bool_xor([]) == false
    if (isReified()) {
      fixReified(false);
    } else if (shouldHold()) {
      throw InconsistencyException(
          "ArrayBoolOrNode::updateState constraint is violated");
    }
    setState(InvariantNodeState::SUBSUMED);
  }
  if (staticInputVarNodeIds().size() <= 2 && !isReified()) {
    if (staticInputVarNodeIds().size() == 1) {
      invariantGraph()
          .varNode(staticInputVarNodeIds().front())
          .fixToValue(shouldHold());
      setState(InvariantNodeState::SUBSUMED);
    } else if (trueIndex < staticInputVarNodeIds().size()) {
      const size_t fixIndex = trueIndex == 0 ? 1 : 0;
      invariantGraph()
          .varNode(staticInputVarNodeIds().at(fixIndex))
          .fixToValue(!shouldHold());
      setState(InvariantNodeState::SUBSUMED);
    }
  }
}

bool ArrayBoolXorNode::canBeReplaced() const {
  if (state() != InvariantNodeState::ACTIVE) {
    return false;
  }
  if (isReified() && staticInputVarNodeIds().size() == 1) {
    return true;
  }
  if (staticInputVarNodeIds().size() == 2) {
    if (!isReified()) {
      return !shouldHold();
    }
    return std::ranges::count_if(
               staticInputVarNodeIds(), [&](const VarNodeId vId) {
                 return invariantGraphConst().varNodeConst(vId).isFixed() &&
                        invariantGraphConst().varNodeConst(vId).inDomain(
                            bool{true});
               }) == 1;
  }
  return false;
}

bool ArrayBoolXorNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (staticInputVarNodeIds().size() == 1) {
    assert(isReified());
    invariantGraph().replaceVarNode(reifiedViolationNodeId(),
                                    staticInputVarNodeIds().front());
  }
  if (staticInputVarNodeIds().size() == 2) {
    if (!isReified() && !shouldHold()) {
      invariantGraph().addInvariantNode(std::make_shared<BoolAllEqualNode>(
          invariantGraph(), std::vector<VarNodeId>{staticInputVarNodeIds()},
          true));
    } else {
      assert(isReified());
      assert(std::ranges::count_if(
                 staticInputVarNodeIds(), [&](const VarNodeId vId) {
                   return invariantGraphConst().varNodeConst(vId).isFixed() &&
                          invariantGraphConst().varNodeConst(vId).inDomain(
                              bool{true});
                 }) == 1);
      const VarNodeId vId =
          invariantGraphConst()
                      .varNodeConst(staticInputVarNodeIds().front())
                      .isFixed() &&
                  invariantGraphConst()
                      .varNodeConst(staticInputVarNodeIds().front())
                      .inDomain(bool{true})
              ? staticInputVarNodeIds().back()
              : staticInputVarNodeIds().front();
      invariantGraph().addInvariantNode(std::make_shared<BoolNotNode>(
          invariantGraph(), vId, reifiedViolationNodeId()));
    }
  }
  return true;
}

void ArrayBoolXorNode::registerOutputVars() {
  if (staticInputVarNodeIds().size() > 1 &&
      violationVarId() == propagation::NULL_ID) {
    if (staticInputVarNodeIds().size() == 2) {
      assert(isReified() || shouldHold());
      registerViolation();
      return;
    }
    _intermediate = solver().makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(solver().makeIntView<propagation::EqualConst>(
          solver(), _intermediate, 1));
    } else {
      assert(!isReified() && staticInputVarNodeIds().size() > 2);
      setViolationVarId(solver().makeIntView<propagation::NotEqualConst>(
          solver(), _intermediate, 1));
    }
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).varId() !=
               propagation::NULL_ID;
      }));
}

void ArrayBoolXorNode::registerNode() {
  if (staticInputVarNodeIds().size() <= 1) {
    return;
  }
  assert(violationVarId() != propagation::NULL_ID);
  assert(staticInputVarNodeIds().size() == 2 ||
         _intermediate != propagation::NULL_ID);
  assert(staticInputVarNodeIds().size() == 2 ? violationVarId().isVar()
                                             : _intermediate.isVar());

  std::vector<propagation::VarViewId> inputNodeIds;
  std::ranges::transform(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      std::back_inserter(inputNodeIds),
      [&](const auto& node) { return invariantGraph().varId(node); });
  if (staticInputVarNodeIds().size() == 2) {
    assert(isReified() || shouldHold());
    assert(_intermediate == propagation::NULL_ID);
    solver().makeInvariant<propagation::BoolXor>(
        solver(), violationVarId(), inputNodeIds.front(), inputNodeIds.back());
    return;
  }

  solver().makeInvariant<propagation::BoolLinear>(solver(), _intermediate,
                                                  std::move(inputNodeIds));
}

std::string ArrayBoolXorNode::dotLangIdentifier() const {
  return "array_bool_xor";
}

}  // namespace atlantis::invariantgraph
