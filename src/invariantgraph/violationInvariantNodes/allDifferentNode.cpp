#include "atlantis/invariantgraph/violationInvariantNodes/allDifferentNode.hpp"

#include <limits>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/fzn/fzn_all_different_int.hpp"
#include "atlantis/invariantgraph/implicitConstraintNodes/allDifferentImplicitNode.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolAllEqualNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intAllEqualNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/propagation/violationInvariants/allDifferent.hpp"
#include "atlantis/propagation/violationInvariants/notEqual.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

AllDifferentNode::AllDifferentNode(InvariantGraph& graph, VarNodeId a,
                                   VarNodeId b, VarNodeId r)
    : AllDifferentNode(graph, std::vector<VarNodeId>{a, b}, r) {}

AllDifferentNode::AllDifferentNode(InvariantGraph& graph, VarNodeId a,
                                   VarNodeId b, bool shouldHold)
    : AllDifferentNode(graph, std::vector<VarNodeId>{a, b}, shouldHold) {}

AllDifferentNode::AllDifferentNode(InvariantGraph& graph,
                                   std::vector<VarNodeId>&& vars, VarNodeId r)
    : ViolationInvariantNode(graph, std::move(vars), r) {}

AllDifferentNode::AllDifferentNode(InvariantGraph& graph,
                                   std::vector<VarNodeId>&& vars,
                                   bool shouldHold)
    : ViolationInvariantNode(graph, std::move(vars), shouldHold) {}

void AllDifferentNode::init(InvariantNodeId id) {
  ViolationInvariantNode::init(id);
  assert(
      !isReified() ||
      !invariantGraphConst().varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(std::ranges::all_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
}

void AllDifferentNode::updateState() {
  ViolationInvariantNode::updateState();
  if (!isReified() && shouldHold()) {
    const std::vector<VarNodeId> varsToRemove =
        pruneAllDifferentFixed(invariantGraph(), staticInputVarNodeIds());
    for (const auto vId : varsToRemove) {
      removeStaticInputVarNode(vId);
    }
  }
  if (staticInputVarNodeIds().size() <= 1) {
    if (isReified()) {
      fixReified(true);
    } else if (!shouldHold()) {
      throw InconsistencyException(
          "AllDifferentNode neg: one or less input variables");
    }
    setState(InvariantNodeState::SUBSUMED);
  }
  Int unionLb = std::numeric_limits<Int>::max();
  Int unionUb = std::numeric_limits<Int>::min();
  for (const auto vId : staticInputVarNodeIds()) {
    unionLb =
        std::min(unionLb, invariantGraph().varNodeConst(vId).lowerBound());
    unionUb =
        std::max(unionUb, invariantGraph().varNodeConst(vId).upperBound());
  }
  const Int numVals = unionUb - unionLb + 1;
  if (numVals < static_cast<Int>(staticInputVarNodeIds().size())) {
    if (isReified()) {
      fixReified(false);
    } else if (shouldHold()) {
      throw InconsistencyException(
          "AllDifferentNode: the union of the domains is smaller than the "
          "number of variables.");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  bool allDisjoint = true;
  for (size_t i = 0; allDisjoint && i < staticInputVarNodeIds().size(); ++i) {
    const auto& iNode =
        invariantGraphConst().varNodeConst(staticInputVarNodeIds()[i]);
    for (size_t j = i + 1; j < staticInputVarNodeIds().size(); ++j) {
      const auto& jNode =
          invariantGraphConst().varNodeConst(staticInputVarNodeIds()[i]);
      if (!iNode.constDomain()->isDisjoint(*jNode.constDomain())) {
        allDisjoint = false;
        break;
      }
    }
  }
  if (allDisjoint) {
    if (isReified()) {
      fixReified(true);
    } else if (!shouldHold()) {
      throw InconsistencyException(
          "AllDifferentNode neg: domains do not intersect");
    }
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool AllDifferentNode::canBeMadeImplicit() const {
  return state() == InvariantNodeState::ACTIVE && !isReified() &&
         shouldHold() &&
         std::ranges::all_of(staticInputVarNodeIds().begin(),
                             staticInputVarNodeIds().end(),
                             [&](const auto& id) {
                               return invariantGraphConst()
                                   .varNodeConst(id)
                                   .definingNodes()
                                   .empty();
                             });
}

bool AllDifferentNode::makeImplicit() {
  if (!canBeMadeImplicit()) {
    return false;
  }
  invariantGraph().addImplicitConstraintNode(
      std::make_shared<AllDifferentImplicitNode>(
          invariantGraph(), std::vector<VarNodeId>{staticInputVarNodeIds()}));
  return true;
}

bool AllDifferentNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE && !isReified() &&
         !shouldHold() && staticInputVarNodeIds().size() <= 2;
}

bool AllDifferentNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  assert(invariantGraphConst()
             .varNodeConst(staticInputVarNodeIds().front())
             .isIntVar());
  invariantGraph().addInvariantNode(std::make_shared<IntAllEqualNode>(
      invariantGraph(), std::vector<VarNodeId>{staticInputVarNodeIds()}));
  return true;
}

void AllDifferentNode::registerOutputVars() {
  assert(state() == InvariantNodeState::ACTIVE);
  assert(!staticInputVarNodeIds().empty());
  if (!staticInputVarNodeIds().empty() &&
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
  assert(outputVarNodeIds().size() <= 1);
  assert(outputVarNodeIds().empty() ||
         std::ranges::all_of(
             outputVarNodeIds().begin(), outputVarNodeIds().end(),
             [&](const VarNodeId vId) {
               return invariantGraphConst().varNodeConst(vId).varId() !=
                      propagation::NULL_ID;
             }));
}

void AllDifferentNode::registerNode() {
  if (staticInputVarNodeIds().empty()) {
    return;
  }
  assert(violationVarId() != propagation::NULL_ID);
  assert(shouldHold() || _intermediate != propagation::NULL_ID);
  assert(shouldHold() ? violationVarId().isVar() : _intermediate.isVar());

  std::vector<propagation::VarViewId> solverVars;
  solverVars.reserve(staticInputVarNodeIds().size());
  std::ranges::transform(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      std::back_inserter(solverVars),
      [&](const auto& id) { return invariantGraph().varId(id); });

  if (solverVars.size() == 2) {
    solver().makeViolationInvariant<propagation::NotEqual>(
        solver(),
        _intermediate != propagation::NULL_ID ? _intermediate
                                              : violationVarId(),
        solverVars.front(), solverVars.back());
  } else {
    solver().makeViolationInvariant<propagation::AllDifferent>(
        solver(),
        _intermediate != propagation::NULL_ID ? _intermediate
                                              : violationVarId(),
        std::move(solverVars));
  }
}

std::string AllDifferentNode::dotLangIdentifier() const {
  return "all_different";
}

}  // namespace atlantis::invariantgraph
