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
    : ViolationInvariantNode(graph, std::vector<std::shared_ptr<VarNode>>{a.ptr(), b.ptr()},
                             output) {}

ArrayBoolAndNode::ArrayBoolAndNode(InvariantGraph& graph, VarNode& a,
                                   VarNode& b, const bool shouldHold)
    : ViolationInvariantNode(graph, std::vector<std::shared_ptr<VarNode>>{a.ptr(), b.ptr()},
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
  assert(!isReified() || !reifiedViolationNode()->isIntVar());
  assert(std::ranges::none_of(staticInputVarNodes(),
                              [&](const std::shared_ptr<VarNode>& vNode) {
                                return vNode->isIntVar();
                              }));
}

void ArrayBoolAndNode::postConstraint() {
  ViolationInvariantNode::postConstraint();
  if (isReified()) {
    constraintSolver().array_bool_and(
        toConstraintVarIds(invariantGraphConst(), staticInputVarNodes()),
        reifiedViolationNode()->constraintVarId());
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
                              [&](const std::shared_ptr<VarNode>& vNode) {
                                return vNode->isFixed() &&
                                       vNode->inDomain(true);
                              });
    } else {
      alwaysHolds =
          !staticInputVarNodes().empty() &&
          std::ranges::any_of(staticInputVarNodes(),
                              [&](const std::shared_ptr<VarNode>& vNode) {
                                return vNode->isFixed() &&
                                       vNode->inDomain(false);
                              });
    }
    if (alwaysHolds) {
      setState(InvariantNodeState::SUBSUMED);
      return;
    }
  }

  std::vector<std::shared_ptr<VarNode>> varsToRemove;
  varsToRemove.reserve(staticInputVarNodes().size());
  for (const auto& vNode : staticInputVarNodes()) {
    if (vNode->isFixed()) {
      varsToRemove.emplace_back(vNode);
    }
  }
  for (const auto& vNode : varsToRemove) {
    removeStaticInputVarNode(*vNode);
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
    invariantGraph().replaceVarNode(*reifiedViolationNode(),
                                    staticInputVarNode(0));
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
      mapping.setIntermediateId(ptrConst(), solver.makeIntVar(0, 0, 0));
      setViolationVarId(solver.makeIntView<propagation::NotEqualConst>(
                            solver, mapping.intermediateId(ptrConst()), 0),
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
  assert(shouldHold() || mapping.intermediateId(ptrConst()) != propagation::NULL_ID);
  assert(shouldHold() ? violationVarId(mapping).isVar()
                      : mapping.intermediateId(ptrConst()).isVar());

  std::vector<propagation::VarViewId> solverVars;
  solverVars.reserve(staticInputVarNodes().size());
  std::ranges::transform(
      staticInputVarNodes(), std::back_inserter(solverVars),
      [&](const auto& node) { return mapping.solverId(node); });
  if (solverVars.size() == 2) {
    solver.makeInvariant<propagation::BoolAnd>(
        solver,
        !shouldHold() ? mapping.intermediateId(ptrConst()) : violationVarId(mapping),
        solverVars.front(), solverVars.back());
  } else {
    solver.makeInvariant<propagation::Max>(
        solver,
        !shouldHold() ? mapping.intermediateId(ptrConst()) : violationVarId(mapping),
        std::move(solverVars));
  }
}

std::string ArrayBoolAndNode::dotLangIdentifier() const {
  return "array_bool_and";
}

}  // namespace atlantis::invariantgraph
