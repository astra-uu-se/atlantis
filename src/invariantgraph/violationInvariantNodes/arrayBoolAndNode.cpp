#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolAndNode.hpp"

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/views/boolNotNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolEqNode.hpp"
#include "atlantis/propagation/invariants/boolAnd.hpp"
#include "atlantis/propagation/invariants/forAll.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {

ArrayBoolAndNode::ArrayBoolAndNode(VarNodeId a, VarNodeId b, VarNodeId output)
    : ViolationInvariantNode(std::vector<VarNodeId>{a, b}, output) {}

ArrayBoolAndNode::ArrayBoolAndNode(VarNodeId a, VarNodeId b, bool shouldHold)
    : ViolationInvariantNode(std::vector<VarNodeId>{a, b}, shouldHold) {}

ArrayBoolAndNode::ArrayBoolAndNode(std::vector<VarNodeId>&& as,
                                   VarNodeId output)
    : ViolationInvariantNode(std::move(as), output) {}

ArrayBoolAndNode::ArrayBoolAndNode(std::vector<VarNodeId>&& as, bool shouldHold)
    : ViolationInvariantNode(std::move(as), shouldHold) {}

void ArrayBoolAndNode::updateState(InvariantGraph& invariantGraph) {
  ViolationInvariantNode::updateState(invariantGraph);
  if (isReified()) {
    return;
  }
  if (!shouldHold()) {
    for (const auto& id : staticInputVarNodeIds()) {
      if (invariantGraph.varNodeConst(id).isFixed() &&
          !invariantGraph.varNodeConst(id).inDomain(false)) {
        if (isReified()) {
          invariantGraph.varNode(reifiedViolationNodeId()).fixToValue(false);
        }
        setState(InvariantNodeState::SUBSUMED);
        return;
      }
    }
    return;
  }
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());
  for (const auto& id : staticInputVarNodeIds()) {
    if (invariantGraph.varNodeConst(id).isFixed() &&
        invariantGraph.varNodeConst(id).inDomain(true)) {
      varsToRemove.push_back(id);
    }
  }
  for (const auto& id : varsToRemove) {
    removeStaticInputVarNode(invariantGraph.varNode(id));
  }
  if (staticInputVarNodeIds().size() == 0) {
    if (isReified()) {
      invariantGraph.varNode(reifiedViolationNodeId()).fixToValue(true);
    }
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool ArrayBoolAndNode::canBeReplaced(const InvariantGraph&) const {
  return state() == InvariantNodeState::ACTIVE &&
         staticInputVarNodeIds().size() <= 1;
}

bool ArrayBoolAndNode::replace(InvariantGraph& graph) {
  if (!canBeReplaced(graph)) {
    return false;
  }
  assert(staticInputVarNodeIds().size() <= 1);
  if (staticInputVarNodeIds().size() == 1) {
    if (isReified()) {
      graph.addInvariantNode(std::make_unique<BoolEqNode>(
          staticInputVarNodeIds().front(), reifiedViolationNodeId()));
    } else if (shouldHold()) {
      graph.varNode(staticInputVarNodeIds().front()).setIsViolationVar(true);
    } else {
      const VarNodeId neg = graph.retrieveBoolVarNode();
      graph.varNode(neg).setIsViolationVar(true);
      graph.addInvariantNode(
          std::make_unique<BoolNotNode>(staticInputVarNodeIds().front(), neg));
    }
  }
  return true;
}

void ArrayBoolAndNode::registerOutputVars(InvariantGraph& invariantGraph,
                                          propagation::SolverBase& solver) {
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

void ArrayBoolAndNode::registerNode(InvariantGraph& invariantGraph,
                                    propagation::SolverBase& solver) {
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);
  std::vector<propagation::VarId> solverVars;
  solverVars.reserve(staticInputVarNodeIds().size());
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars),
                 [&](const auto& node) { return invariantGraph.varId(node); });
  if (solverVars.size() == 2) {
    solver.makeInvariant<propagation::BoolAnd>(
        solver, !shouldHold() ? _intermediate : violationVarId(invariantGraph),
        solverVars.front(), solverVars.back());
  } else {
    solver.makeInvariant<propagation::ForAll>(
        solver, !shouldHold() ? _intermediate : violationVarId(invariantGraph),
        std::move(solverVars));
  }
}

}  // namespace atlantis::invariantgraph
