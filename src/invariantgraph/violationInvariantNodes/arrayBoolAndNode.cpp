#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolAndNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/views/boolNotNode.hpp"
#include "atlantis/propagation/invariants/boolAnd.hpp"
#include "atlantis/propagation/invariants/max.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {

ArrayBoolAndNode::ArrayBoolAndNode(InvariantGraph& graph, VarNodeId a,
                                   VarNodeId b, VarNodeId output)
    : ViolationInvariantNode(graph, std::vector<VarNodeId>{a, b}, output) {}

ArrayBoolAndNode::ArrayBoolAndNode(InvariantGraph& graph, VarNodeId a,
                                   VarNodeId b, bool shouldHold)
    : ViolationInvariantNode(graph, std::vector<VarNodeId>{a, b}, shouldHold) {}

ArrayBoolAndNode::ArrayBoolAndNode(InvariantGraph& graph,
                                   std::vector<VarNodeId>&& as,
                                   VarNodeId output)
    : ViolationInvariantNode(graph, std::move(as), output) {}

ArrayBoolAndNode::ArrayBoolAndNode(InvariantGraph& graph,
                                   std::vector<VarNodeId>&& as, bool shouldHold)
    : ViolationInvariantNode(graph, std::move(as), shouldHold) {}

void ArrayBoolAndNode::init(InvariantNodeId id) {
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

void ArrayBoolAndNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  std::vector<ConstraintVarId> inputs(staticInputVarNodeIds().size());
  for (size_t i = 0; i < staticInputVarNodeIds().size(); i++) {
    inputs[i] = invariantGraphConst().varNodeConst(staticInputVarNodeIds()[i]).constraintVarId();
  }
  if (isReified()) {
    constraintSolver().array_bool_and(inputs, invariantGraphConst().varNodeConst(reifiedViolationNodeId()).constraintVarId());
  } else {
    constraintSolver().array_bool_and(inputs, shouldHold());
  }
}

void ArrayBoolAndNode::updateState() {
  ViolationInvariantNode::updateState();

  // Constraint has subsumed:
  if (!isReified()) {
    bool alwaysHolds = false;
    if (shouldHold()) {
      alwaysHolds = std::ranges::all_of(staticInputVarNodeIds(), [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isFixed() && invariantGraphConst().varNodeConst(vId).inDomain(true);
      });
    } else {
      alwaysHolds = std::ranges::any_of(staticInputVarNodeIds(), [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isFixed() && invariantGraphConst().varNodeConst(vId).inDomain(false);
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
    if (invariantGraphConst().varNodeConst(id).isFixed()) {
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

bool ArrayBoolAndNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE && isReified() &&
         staticInputVarNodeIds().size() == 1;
}

bool ArrayBoolAndNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (staticInputVarNodeIds().size() == 1 && isReified()) {
    invariantGraph().replaceVarNode(reifiedViolationNodeId(),
                                    staticInputVarNodeIds().front());
  }
  return true;
}

void ArrayBoolAndNode::registerOutputVars(propagation::SolverBase& solver,
                                          SolverMapping& mapping) const {
  if (staticInputVarNodeIds().size() > 1 &&
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
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void ArrayBoolAndNode::registerNode(propagation::SolverBase& solver,
                                    SolverMapping& mapping) const {
  if (staticInputVarNodeIds().size() <= 1) {
    return;
  }
  assert(violationVarId(mapping) != propagation::NULL_ID);
  assert(shouldHold() || mapping.intermediateId(id()) != propagation::NULL_ID);
  assert(shouldHold() ? violationVarId(mapping).isVar()
                      : mapping.intermediateId(id()).isVar());

  std::vector<propagation::VarViewId> solverVars;
  solverVars.reserve(staticInputVarNodeIds().size());
  std::ranges::transform(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      std::back_inserter(solverVars),
      [&](const auto& node) { return mapping.solverId(node); });
  if (solverVars.size() == 2) {
    solver.makeInvariant<propagation::BoolAnd>(
        solver,
        !shouldHold() ? mapping.intermediateId(id()) : violationVarId(mapping),
        solverVars.front(), solverVars.back());
  } else {
    solver.makeInvariant<propagation::Max>(
        solver,
        !shouldHold() ? mapping.intermediateId(id()) : violationVarId(mapping),
        std::move(solverVars));
  }
}

std::string ArrayBoolAndNode::dotLangIdentifier() const {
  return "array_bool_and";
}

}  // namespace atlantis::invariantgraph
