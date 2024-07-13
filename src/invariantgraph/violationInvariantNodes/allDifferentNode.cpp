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

void AllDifferentNode::init(InvariantGraph& graph, const InvariantNodeId& id) {
  ViolationInvariantNode::init(graph, id);
  assert(!isReified() ||
         !graph.varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(std::all_of(staticInputVarNodeIds().begin(),
                     staticInputVarNodeIds().end(), [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).isIntVar();
                     }));
}

void AllDifferentNode::updateState(InvariantGraph& graph) {
  ViolationInvariantNode::updateState(graph);
  if (isReified() || !shouldHold()) {
    return;
  }
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());
  for (const auto& id : staticInputVarNodeIds()) {
    if (graph.varNodeConst(id).isFixed()) {
      varsToRemove.push_back(id);
    }
  }
  for (const auto& id : varsToRemove) {
    removeStaticInputVarNode(graph.varNode(id));
  }
  if (staticInputVarNodeIds().size() <= 1) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool AllDifferentNode::canBeMadeImplicit(const InvariantGraph& graph) const {
  return state() == InvariantNodeState::ACTIVE && !isReified() &&
         shouldHold() &&
         std::all_of(staticInputVarNodeIds().begin(),
                     staticInputVarNodeIds().end(), [&](const auto& id) {
                       return graph.varNodeConst(id).definingNodes().empty();
                     });
}

bool AllDifferentNode::makeImplicit(InvariantGraph& graph) {
  if (!canBeMadeImplicit(graph)) {
    return false;
  }
  graph.addImplicitConstraintNode(std::make_unique<AllDifferentImplicitNode>(
      std::vector<VarNodeId>{staticInputVarNodeIds()}));
  return true;
}

void AllDifferentNode::registerOutputVars(InvariantGraph& graph,
                                          propagation::SolverBase& solver) {
  assert(state() == InvariantNodeState::ACTIVE);
  assert(!staticInputVarNodeIds().empty());
  if (!staticInputVarNodeIds().empty() &&
      violationVarId(graph) == propagation::NULL_ID) {
    if (shouldHold()) {
      registerViolation(graph, solver);
    } else {
      assert(!isReified());
      _intermediate = solver.makeIntVar(0, 0, 0);
      setViolationVarId(graph, solver.makeIntView<propagation::NotEqualConst>(
                                   solver, _intermediate, 0));
    }
  }
  assert(outputVarNodeIds().size() <= 1);
  assert(outputVarNodeIds().empty() ||
         std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void AllDifferentNode::registerNode(InvariantGraph& graph,
                                    propagation::SolverBase& solver) {
  if (staticInputVarNodeIds().empty()) {
    return;
  }
  assert(violationVarId(graph) != propagation::NULL_ID);

  std::vector<propagation::VarId> solverVars;
  solverVars.reserve(staticInputVarNodeIds().size());
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars),
                 [&](const auto& id) { return graph.varId(id); });

  if (solverVars.size() == 2) {
    solver.makeViolationInvariant<propagation::NotEqual>(
        solver, !shouldHold() ? _intermediate : violationVarId(graph),
        solverVars.front(), solverVars.back());
  } else {
    solver.makeViolationInvariant<propagation::AllDifferent>(
        solver, !shouldHold() ? _intermediate : violationVarId(graph),
        std::move(solverVars));
  }
}

}  // namespace atlantis::invariantgraph
