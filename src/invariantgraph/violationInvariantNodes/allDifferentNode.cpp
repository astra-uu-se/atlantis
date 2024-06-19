#include "atlantis/invariantgraph/violationInvariantNodes/allDifferentNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/implicitConstraintNodes/allDifferentImplicitNode.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/propagation/violationInvariants/allDifferent.hpp"
#include "atlantis/propagation/violationInvariants/notEqual.hpp"

namespace atlantis::invariantgraph {

AllDifferentNode::AllDifferentNode(VarNodeId a, VarNodeId b, VarNodeId r)
    : AllDifferentNode(std::vector<VarNodeId>{a, b}, r) {}

AllDifferentNode::AllDifferentNode(VarNodeId a, VarNodeId b, bool shouldHold)
    : AllDifferentNode(std::vector<VarNodeId>{a, b}, shouldHold) {}

AllDifferentNode::AllDifferentNode(std::vector<VarNodeId>&& vars, VarNodeId r)
    : ViolationInvariantNode(std::move(vars), r) {}

AllDifferentNode::AllDifferentNode(std::vector<VarNodeId>&& vars,
                                   bool shouldHold)
    : ViolationInvariantNode(std::move(vars), shouldHold) {}

void AllDifferentNode::updateState(InvariantGraph& invariantGraph) {
  ViolationInvariantNode::updateState(invariantGraph);
  if (isReified() || !shouldHold()) {
    return;
  }
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());
  for (const auto& id : staticInputVarNodeIds()) {
    if (invariantGraph.varNodeConst(id).isFixed()) {
      varsToRemove.push_back(id);
    }
  }
  for (const auto& id : varsToRemove) {
    removeStaticInputVarNode(invariantGraph.varNode(id));
  }
  if (staticInputVarNodeIds().size() <= 1) {
    if (isReified()) {
      invariantGraph.varNode(reifiedViolationNodeId()).fixToValue(true);
    }
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool AllDifferentNode::canBeReplaced(
    const InvariantGraph& invariantGraph) const {
  if (staticInputVarNodeIds().size() <= 1) {
    return true;
  }
  if (isReified() || !shouldHold()) {
    return false;
  }
  return std::all_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const auto& id) {
        return invariantGraph.varNodeConst(id).definingNodes().empty();
      });
}

bool AllDifferentNode::replace(InvariantGraph& invariantGraph) {
  if (!canBeReplaced(invariantGraph)) {
    return false;
  }
  if (staticInputVarNodeIds().size() <= 1) {
    return true;
  }
  assert(std::all_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const auto& id) {
        return invariantGraph.varNodeConst(id).definingNodes().empty();
      }));
  invariantGraph.addImplicitConstraintNode(
      std::make_unique<AllDifferentImplicitNode>(
          std::vector<VarNodeId>{staticInputVarNodeIds()}));
  return true;
}

void AllDifferentNode::registerOutputVars(InvariantGraph& invariantGraph,
                                          propagation::SolverBase& solver) {
  if (staticInputVarNodeIds().empty()) {
    return;
  }
  if (violationVarId(invariantGraph) == propagation::NULL_ID) {
    if (shouldHold()) {
      registerViolation(invariantGraph, solver);
    } else {
      assert(!isReified());
      _intermediate = solver.makeIntVar(0, 0, 0);
      setViolationVarId(invariantGraph,
                        solver.makeIntView<propagation::NotEqualConst>(
                            solver, _intermediate, 0));
    }
  }
}

void AllDifferentNode::registerNode(InvariantGraph& invariantGraph,
                                    propagation::SolverBase& solver) {
  if (staticInputVarNodeIds().empty()) {
    return;
  }
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);

  std::vector<propagation::VarId> solverVars;
  solverVars.reserve(staticInputVarNodeIds().size());
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars),
                 [&](const auto& id) { return invariantGraph.varId(id); });

  if (solverVars.size() == 2) {
    solver.makeViolationInvariant<propagation::NotEqual>(
        solver, !shouldHold() ? _intermediate : violationVarId(invariantGraph),
        solverVars.front(), solverVars.back());
  } else {
    solver.makeViolationInvariant<propagation::AllDifferent>(
        solver, !shouldHold() ? _intermediate : violationVarId(invariantGraph),
        std::move(solverVars));
  }
}

}  // namespace atlantis::invariantgraph
