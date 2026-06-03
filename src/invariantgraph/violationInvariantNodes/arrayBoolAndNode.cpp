#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolAndNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/views/boolNotNode.hpp"
#include "atlantis/propagation/invariants/boolAnd.hpp"
#include "atlantis/propagation/invariants/max.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {

ArrayBoolAndNode::ArrayBoolAndNode(InvariantGraph& graph, const VarNodeId a,
                                   const VarNodeId b, const VarNodeId output)
    : ViolationInvariantNode(graph, std::vector<VarNodeId>{a, b}, output) {}

ArrayBoolAndNode::ArrayBoolAndNode(InvariantGraph& graph, const VarNodeId a,
                                   const VarNodeId b, const bool shouldHold)
    : ViolationInvariantNode(graph, std::vector<VarNodeId>{a, b}, shouldHold) {}

ArrayBoolAndNode::ArrayBoolAndNode(InvariantGraph& graph,
                                   std::vector<VarNodeId>&& as,
                                   const VarNodeId output)
    : ViolationInvariantNode(graph, std::move(as), output) {}

ArrayBoolAndNode::ArrayBoolAndNode(InvariantGraph& graph,
                                   std::vector<VarNodeId>&& as,
                                   const bool shouldHold)
    : ViolationInvariantNode(graph, std::move(as), shouldHold) {}

void ArrayBoolAndNode::init(const InvariantNodeId id) {
  ViolationInvariantNode::init(id);
  assert(!isReified() || !reifiedVarNodeConst().isIntVar());
  assert(std::ranges::none_of(
      staticInputVarNodeIds(),
      [&](const VarNodeId vId) { return varNodeConst(vId).isIntVar(); }));
}

void ArrayBoolAndNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  if (staticInputVarNodeIds().size() < 2) {
    return;
  }
  if (staticInputVarNodeIds().size() == 2) {
    if (isReified()) {
      constraintSolver().bool_and_reif(
          staticInputVarNodeConst(0).constraintVarId(),
          staticInputVarNodeConst(1).constraintVarId(),
          reifiedVarNodeConst().constraintVarId());
    } else {
      constraintSolver().bool_and(staticInputVarNodeConst(0).constraintVarId(),
                                  staticInputVarNodeConst(1).constraintVarId(),
                                  shouldHold());
    }
    return;
  }
  if (isReified()) {
    constraintSolver().array_bool_and(
        toConstraintVarIds(invariantGraphConst(), staticInputVarNodeIds()),
        reifiedVarNodeConst().constraintVarId());
  } else {
    constraintSolver().array_bool_and(
        toConstraintVarIds(invariantGraphConst(), staticInputVarNodeIds()),
        shouldHold());
  }
}

void ArrayBoolAndNode::updateState() {
  ViolationInvariantNode::updateState();

  // Constraint has subsumed:
  if (!isReified()) {
    bool alwaysHolds = false;
    if (shouldHold()) {
      alwaysHolds = std::ranges::all_of(
          staticInputVarNodeIds(), [&](const VarNodeId vId) {
            return varNodeConst(vId).isFixed() &&
                   varNodeConst(vId).inDomain(true);
          });
    } else {
      alwaysHolds = std::ranges::any_of(
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
  assert(std::ranges::all_of(outputVarNodeIds(), [&](const VarNodeId vId) {
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
      staticInputVarNodeIds(), std::back_inserter(solverVars),
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
