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

ArrayBoolAndNode::ArrayBoolAndNode(InvariantGraph& graph, VarNode& a,
                                   VarNode& b, VarNode& output)
    : ViolationInvariantNode(graph, std::vector<std::shared_ptr<VarNode>>{a, b},
                             output) {}

ArrayBoolAndNode::ArrayBoolAndNode(InvariantGraph& graph, VarNode& a,
                                   VarNode& b, const bool shouldHold)
    : ViolationInvariantNode(graph, std::vector<std::shared_ptr<VarNode>>{a, b},
                             shouldHold) {}

ArrayBoolAndNode::ArrayBoolAndNode(InvariantGraph& graph,
                                   std::vector<std::shared_ptr<VarNode>>&& as,
                                   VarNode& output)
    : ViolationInvariantNode(graph, std::move(as), output) {}

ArrayBoolAndNode::ArrayBoolAndNode(InvariantGraph& graph,
                                   std::vector<std::shared_ptr<VarNode>>&& as,
                                   const bool shouldHold)
    : ViolationInvariantNode(graph, std::move(as), shouldHold) {}

void ArrayBoolAndNode::init() {
  ViolationInvariantNode::init();
  assert(!isReified() || !reifiedVarNodeConst().isIntVar());
  assert(std::ranges::none_of(staticInputVarNodes(),
                              [&](const std::shared_ptr<VarNode>& vId) {
                                return varNodeConst(vId).isIntVar();
                              }));
}

void ArrayBoolAndNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  if (isReified()) {
    constraintSolver().array_bool_and(
        toConstraintVarIds(invariantGraphConst(), staticInputVarNodes()),
        reifiedVarNodeConst().constraintVarId());
  } else {
    constraintSolver().array_bool_and(
        toConstraintVarIds(invariantGraphConst(), staticInputVarNodes()),
        shouldHold());
  }
}

void ArrayBoolAndNode::updateState() {
  ViolationInvariantNode::updateState();

  if (!isReified()) {
    bool alwaysHolds = false;
    if (shouldHold()) {
      alwaysHolds =
          staticInputVarNodes().empty() ||
          std::ranges::all_of(staticInputVarNodes(),
                              [&](const std::shared_ptr<VarNode>& vId) {
                                return varNodeConst(vId).isFixed() &&
                                       varNodeConst(vId).inDomain(true);
                              });
    } else {
      alwaysHolds =
          !staticInputVarNodes().empty() &&
          std::ranges::any_of(staticInputVarNodes(),
                              [&](const std::shared_ptr<VarNode>& vId) {
                                return varNodeConst(vId).isFixed() &&
                                       varNodeConst(vId).inDomain(false);
                              });
    }
    if (alwaysHolds) {
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
  }

  std::vector<std::shared_ptr<VarNode>> varsToRemove;
  varsToRemove.reserve(staticInputVarNodes().size());
  for (const auto& id : staticInputVarNodes()) {
    if (varNodeConst(id).isFixed()) {
      varsToRemove.emplace_back(id);
    }
  }
  for (const auto& id : varsToRemove) {
    removeStaticInputVarNode(id);
  }
  if (staticInputVarNodes().empty()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool ArrayBoolAndNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE && isReified() &&
         staticInputVarNodes().size() == 1;
}

bool ArrayBoolAndNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (staticInputVarNodes().size() == 1 && isReified()) {
    invariantGraph().replaceVarNode(reifiedViolationNode(),
                                    staticInputVarNodes().front());
  }
  return true;
}

void ArrayBoolAndNode::registerOutputVars(propagation::SolverBase& solver,
                                          SolverMapping& mapping) const {
  if (staticInputVarNodes().size() > 1 &&
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
      outputVarNodes(), [&](const std::shared_ptr<VarNode>& vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void ArrayBoolAndNode::registerNode(propagation::SolverBase& solver,
                                    SolverMapping& mapping) const {
  if (staticInputVarNodes().size() <= 1) {
    return;
  }
  assert(violationVarId(mapping) != propagation::NULL_ID);
  assert(shouldHold() || mapping.intermediateId(id()) != propagation::NULL_ID);
  assert(shouldHold() ? violationVarId(mapping).isVar()
                      : mapping.intermediateId(id()).isVar());

  std::vector<propagation::VarViewId> solverVars;
  solverVars.reserve(staticInputVarNodes().size());
  std::ranges::transform(
      staticInputVarNodes(), std::back_inserter(solverVars),
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
