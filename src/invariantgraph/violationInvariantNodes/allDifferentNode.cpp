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
#include "atlantis/utils/overflow.hpp"

namespace atlantis::invariantgraph {

AllDifferentNode::AllDifferentNode(InvariantGraph& graph, const VarNodeId a,
                                   const VarNodeId b, const VarNodeId r)
    : AllDifferentNode(graph, std::vector<VarNodeId>{a, b}, r) {}

AllDifferentNode::AllDifferentNode(InvariantGraph& graph, const VarNodeId a,
                                   const VarNodeId b, const bool shouldHold)
    : AllDifferentNode(graph, std::vector<VarNodeId>{a, b}, shouldHold) {}

AllDifferentNode::AllDifferentNode(InvariantGraph& graph,
                                   std::vector<VarNodeId>&& vars,
                                   const VarNodeId r)
    : ViolationInvariantNode(graph, std::move(vars), r) {}

AllDifferentNode::AllDifferentNode(InvariantGraph& graph,
                                   std::vector<VarNodeId>&& vars,
                                   const bool shouldHold)
    : ViolationInvariantNode(graph, std::move(vars), shouldHold) {}

void AllDifferentNode::init(const InvariantNodeId id) {
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

void AllDifferentNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  if (staticInputVarNodeIds().size() == 2) {
    if (isReified()) {
      return constraintSolver().int_ne_reif(
          staticInputVarNodeConst(0).constraintVarId(),
          staticInputVarNodeConst(1).constraintVarId(),
          reifiedVarNodeConst().constraintVarId());
    }
    return constraintSolver().int_ne(
        staticInputVarNodeConst(0).constraintVarId(),
        staticInputVarNodeConst(1).constraintVarId(), shouldHold());
  }
  if (isReified()) {
    return constraintSolver().fzn_all_different_int_reif(
        toConstraintVarIds(invariantGraphConst(), staticInputVarNodeIds()),
        reifiedVarNodeConst().constraintVarId());
  }
  return constraintSolver().fzn_all_different_int(
      toConstraintVarIds(invariantGraphConst(), staticInputVarNodeIds()),
      shouldHold());
}

void AllDifferentNode::updateState() {
  ViolationInvariantNode::updateState();
  if (isReified()) {
    return;
  }
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());
  if (!shouldHold()) {
    for (const auto vId : staticInputVarNodeIds()) {
      if (!varNodeConst(vId).isFixed()) {
        continue;
      }
      for (const Int val : _seenValues) {
        if (val == varNodeConst(vId).lowerBound()) {
          setState(InvariantNodeState::SUBSUMED);
          return;
        }
      }
      _seenValues.emplace_back(varNodeConst(vId).lowerBound());
      varsToRemove.emplace_back(vId);
    }
  } else {
    for (const auto vId : staticInputVarNodeIds()) {
      if (varNodeConst(vId).isFixed()) {
        varsToRemove.emplace_back(vId);
      }
    }
  }

  for (const auto vId : varsToRemove) {
    removeStaticInputVarNode(vId);
  }

  if (staticInputVarNodeIds().size() <= 1) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool AllDifferentNode::canBeMadeImplicit() const {
  return state() == InvariantNodeState::ACTIVE && !isReified() &&
         shouldHold() &&
         std::ranges::all_of(staticInputVarNodeIds(), [&](const auto& id) {
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

void AllDifferentNode::registerOutputVars(propagation::SolverBase& solver,
                                          SolverMapping& mapping) const {
  assert(state() == InvariantNodeState::ACTIVE);
  assert(!staticInputVarNodeIds().empty());
  if (!staticInputVarNodeIds().empty() &&
      violationVarId(mapping) == propagation::NULL_ID) {
    if (shouldHold()) {
      registerViolation(solver, mapping);
    } else {
      assert(!isReified());
      mapping.setIntermediateId(id(), solver.makeIntVar(0, 0, 0));
      setViolationVarId(solver.makeIntView<propagation::NotEqualConst>(
                            solver, mapping.intermediateId(id()), 0),
                        mapping);
    }
  }
  assert(outputVarNodeIds().size() <= 1);
  assert(outputVarNodeIds().empty() ||
         std::ranges::all_of(
             outputVarNodeIds().begin(), outputVarNodeIds().end(),
             [&](const VarNodeId vId) {
               return mapping.solverId(vId) != propagation::NULL_ID;
             }));
}

void AllDifferentNode::registerNode(propagation::SolverBase& solver,
                                    SolverMapping& mapping) const {
  if (staticInputVarNodeIds().empty()) {
    return;
  }
  assert(violationVarId(mapping) != propagation::NULL_ID);
  assert(shouldHold() || mapping.intermediateId(id()) != propagation::NULL_ID);
  assert(shouldHold() ? violationVarId(mapping).isVar()
                      : mapping.intermediateId(id()).isVar());

  std::vector<propagation::VarViewId> solverVars;
  solverVars.reserve(staticInputVarNodeIds().size());
  std::ranges::transform(staticInputVarNodeIds().begin(),
                         staticInputVarNodeIds().end(),
                         std::back_inserter(solverVars),
                         [&](const auto& id) { return mapping.solverId(id); });

  if (solverVars.size() == 2) {
    solver.makeViolationInvariant<propagation::NotEqual>(
        solver,
        mapping.intermediateId(id()) != propagation::NULL_ID
            ? mapping.intermediateId(id())
            : violationVarId(mapping),
        solverVars.front(), solverVars.back());
  } else {
    solver.makeViolationInvariant<propagation::AllDifferent>(
        solver,
        mapping.intermediateId(id()) != propagation::NULL_ID
            ? mapping.intermediateId(id())
            : violationVarId(mapping),
        std::move(solverVars));
  }
}

std::string AllDifferentNode::dotLangIdentifier() const {
  return "all_different";
}

}  // namespace atlantis::invariantgraph
