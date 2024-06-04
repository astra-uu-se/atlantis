#include "atlantis/invariantgraph/violationInvariantNodes/allDifferentNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/implicitConstraintNodes/allDifferentImplicitNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intNeNode.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/propagation/violationInvariants/allDifferent.hpp"
#include "atlantis/propagation/violationInvariants/notEqual.hpp"

namespace atlantis::invariantgraph {

AllDifferentNode::AllDifferentNode(std::vector<VarNodeId>&& vars, VarNodeId r)
    : ViolationInvariantNode(std::move(vars), r) {}

AllDifferentNode::AllDifferentNode(std::vector<VarNodeId>&& vars,
                                   bool shouldHold)
    : ViolationInvariantNode(std::move(vars), shouldHold) {}

void AllDifferentNode::updateVariableNodes(InvariantGraph& invariantGraph) {
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
}

bool AllDifferentNode::canBeReplaced(
    const InvariantGraph& invariantGraph) const {
  if (staticInputVarNodeIds().size() <= 2) {
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
    deactivate(invariantGraph);
    return true;
  }
  if (staticInputVarNodeIds().size() == 2) {
    if (isReified()) {
      invariantGraph.addInvariantNode(std::make_unique<IntNeNode>(
          staticInputVarNodeIds().front(), staticInputVarNodeIds().back(),
          outputVarNodeIds().front()));
    } else {
      invariantGraph.addInvariantNode(std::make_unique<IntNeNode>(
          staticInputVarNodeIds().front(), staticInputVarNodeIds().back(),
          shouldHold()));
    }
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
  deactivate(invariantGraph);
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
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars),
                 [&](const auto& id) { return invariantGraph.varId(id); });

  solver.makeViolationInvariant<propagation::AllDifferent>(
      solver, !shouldHold() ? _intermediate : violationVarId(invariantGraph),
      std::move(solverVars));
}

}  // namespace atlantis::invariantgraph
