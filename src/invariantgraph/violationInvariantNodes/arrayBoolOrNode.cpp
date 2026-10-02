#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolOrNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/views/boolNotNode.hpp"
#include "atlantis/propagation/invariants/boolOr.hpp"
#include "atlantis/propagation/invariants/min.hpp"
#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::invariantgraph {

ArrayBoolOrNode::ArrayBoolOrNode(InvariantGraph& graph, VarNode& a, VarNode& b,
                                 VarNode& reified)
    : ViolationInvariantNode(graph, std::vector<std::shared_ptr<VarNode>>{a, b},
                             reified) {}

ArrayBoolOrNode::ArrayBoolOrNode(InvariantGraph& graph, VarNode& a, VarNode& b,
                                 const bool shouldHold)
    : ViolationInvariantNode(graph, std::vector<std::shared_ptr<VarNode>>{a, b},
                             shouldHold) {}

ArrayBoolOrNode::ArrayBoolOrNode(InvariantGraph& graph,
                                 std::vector<std::shared_ptr<VarNode>>&& inputs,
                                 VarNode& reified)
    : ViolationInvariantNode(graph, std::move(inputs), reified) {}

ArrayBoolOrNode::ArrayBoolOrNode(InvariantGraph& graph,
                                 std::vector<std::shared_ptr<VarNode>>&& inputs,
                                 const bool shouldHold)
    : ViolationInvariantNode(graph, std::move(inputs), shouldHold) {}

void ArrayBoolOrNode::init() {
  ViolationInvariantNode::init();
  assert(!isReified() || !reifiedVarNodeConst().isIntVar());
  assert(std::ranges::none_of(staticInputVarNodes(),
                              [&](const std::shared_ptr<VarNode>& vId) {
                                return varNodeConst(vId).isIntVar();
                              }));
}

void ArrayBoolOrNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  if (isReified()) {
    constraintSolver().array_bool_or(
        toConstraintVarIds(invariantGraphConst(), staticInputVarNodes()),
        reifiedVarNodeConst().constraintVarId());
  } else {
    constraintSolver().array_bool_or(
        toConstraintVarIds(invariantGraphConst(), staticInputVarNodes()),
        shouldHold());
  }
}

void ArrayBoolOrNode::updateState() {
  ViolationInvariantNode::updateState();
  if (!isReified()) {
    bool alwaysHolds = false;
    if (shouldHold()) {
      alwaysHolds = std::ranges::any_of(
          staticInputVarNodes(), [&](const std::shared_ptr<VarNode>& vId) {
            return varNodeConst(vId).isFixed() &&
                   varNodeConst(vId).inDomain(true);
          });
    } else {
      alwaysHolds = std::ranges::all_of(
          staticInputVarNodes(), [&](const std::shared_ptr<VarNode>& vId) {
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

bool ArrayBoolOrNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE && isReified() &&
         staticInputVarNodes().size() == 1;
}

bool ArrayBoolOrNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (staticInputVarNodes().size() == 1 && isReified()) {
    invariantGraph().replaceVarNode(reifiedViolationNode(),
                                    staticInputVarNodes().front());
  }
  return true;
}

void ArrayBoolOrNode::registerOutputVars(propagation::SolverBase& solver,
                                         SolverMapping& mapping) const {
  if (staticInputVarNodes().size() > 1 && shouldHold() &&
      violationVarId(mapping) == propagation::NULL_ID) {
    registerViolation(solver, mapping);
  }
  assert(std::ranges::all_of(
      outputVarNodes(), [&](const std::shared_ptr<VarNode>& vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void ArrayBoolOrNode::registerNode(propagation::SolverBase& solver,
                                   SolverMapping& mapping) const {
  if (staticInputVarNodes().size() <= 1 || (!isReified() && !shouldHold())) {
    return;
  }
  assert(violationVarId(mapping) != propagation::NULL_ID);
  assert(shouldHold());
  assert(violationVarId(mapping).isVar());

  std::vector<propagation::VarViewId> solverVars;
  std::ranges::transform(
      staticInputVarNodes(), std::back_inserter(solverVars),
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
