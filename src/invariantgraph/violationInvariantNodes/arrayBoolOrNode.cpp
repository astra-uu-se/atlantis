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
#include "atlantis/invariantgraph/constraintSolver.hpp"

namespace atlantis::invariantgraph {

ArrayBoolOrNode::ArrayBoolOrNode(InvariantGraph& graph, const VarNodeId a,
                                 const VarNodeId b, const VarNodeId reified)
    : ViolationInvariantNode(graph, std::vector<VarNodeId>{a, b}, reified) {}

ArrayBoolOrNode::ArrayBoolOrNode(InvariantGraph& graph, const VarNodeId a,
                                 const VarNodeId b, const bool shouldHold)
    : ViolationInvariantNode(graph, std::vector<VarNodeId>{a, b}, shouldHold) {}

ArrayBoolOrNode::ArrayBoolOrNode(InvariantGraph& graph,
                                 std::vector<VarNodeId>&& inputs,
                                 const VarNodeId reified)
    : ViolationInvariantNode(graph, std::move(inputs), reified) {}

ArrayBoolOrNode::ArrayBoolOrNode(InvariantGraph& graph,
                                 std::vector<VarNodeId>&& inputs,
                                 const bool shouldHold)
    : ViolationInvariantNode(graph, std::move(inputs), shouldHold) {}

void ArrayBoolOrNode::init(const InvariantNodeId id) {
  ViolationInvariantNode::init(id);
  assert(!isReified() || !reifiedVarNodeConst().isIntVar());
  assert(std::ranges::none_of(
      staticInputVarNodeIds(),
      [&](const VarNodeId vId) { return varNodeConst(vId).isIntVar(); }));
}

void ArrayBoolOrNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  if (staticInputVarNodeIds().size() < 2) {
    return;
  }
  if (staticInputVarNodeIds().size() == 2) {
    if (isReified()) {
      constraintSolver().bool_or_reif(
          staticInputVarNodeConst(0).constraintVarId(),
          staticInputVarNodeConst(1).constraintVarId(),
          reifiedVarNodeConst().constraintVarId());
    } else {
      constraintSolver().bool_or(staticInputVarNodeConst(0).constraintVarId(),
                                 staticInputVarNodeConst(1).constraintVarId(),
                                 shouldHold());
    }
    return;
  }
  if (isReified()) {
    constraintSolver().array_bool_or(
        toConstraintVarIds(invariantGraphConst(), staticInputVarNodeIds()),
        reifiedVarNodeConst().constraintVarId());
  } else {
    constraintSolver().array_bool_or(
        toConstraintVarIds(invariantGraphConst(), staticInputVarNodeIds()),
        shouldHold());
  }
}

void ArrayBoolOrNode::updateState() {
  ViolationInvariantNode::updateState();
  if (!isReified()) {
    bool alwaysHolds = false;
    if (shouldHold()) {
      alwaysHolds = std::ranges::any_of(
          staticInputVarNodeIds(), [&](const VarNodeId vId) {
            return varNodeConst(vId).isFixed() &&
                   varNodeConst(vId).inDomain(true);
          });
    } else {
      alwaysHolds = std::ranges::all_of(
          staticInputVarNodeIds(), [&](const VarNodeId vId) {
            return varNodeConst(vId).isFixed() &&
                   varNodeConst(vId).inDomain(false);
          });
    }
    if (alwaysHolds) {
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
  }

  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());
  for (const auto& id : staticInputVarNodeIds()) {
    if (varNodeConst(id).isFixed()) {
      varsToRemove.emplace_back(id);
    }
  }
  for (const auto& id : varsToRemove) {
    removeStaticInputVarNode(id);
  }

  assert(!staticInputVarNodeIds().empty());

  if (staticInputVarNodeIds().empty()) {
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

void ArrayBoolOrNode::registerOutputVars(propagation::SolverBase& solver,
                                         SolverMapping& mapping) const {
  if (staticInputVarNodeIds().size() > 1 && shouldHold() &&
      violationVarId(mapping) == propagation::NULL_ID) {
    registerViolation(solver, mapping);
  }
  assert(std::ranges::all_of(outputVarNodeIds(), [&](const VarNodeId vId) {
    return mapping.solverId(vId) != propagation::NULL_ID;
  }));
}

void ArrayBoolOrNode::registerNode(propagation::SolverBase& solver,
                                   SolverMapping& mapping) const {
  if (staticInputVarNodeIds().size() <= 1 || (!isReified() && !shouldHold())) {
    return;
  }
  assert(violationVarId(mapping) != propagation::NULL_ID);
  assert(shouldHold());
  assert(violationVarId(mapping).isVar());

  std::vector<propagation::VarViewId> solverVars;
  std::ranges::transform(
      staticInputVarNodeIds(), std::back_inserter(solverVars),
      [&](const auto& node) { return mapping.solverId(node); });

  if (solverVars.size() == 2) {
    solver.makeInvariant<propagation::BoolOr>(
        solver, violationVarId(mapping), solverVars.front(), solverVars.back());
  } else {
    solver.makeInvariant<propagation::Min>(solver, violationVarId(mapping),
                                           std::move(solverVars), Int{0});
  }
}

std::string ArrayBoolOrNode::dotLangIdentifier() const {
  return "array_bool_or";
}

}  // namespace atlantis::invariantgraph
