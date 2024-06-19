#include "atlantis/invariantgraph/violationInvariantNodes/boolAllEqualNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolEqNode.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/propagation/violationInvariants/boolAllEqual.hpp"

namespace atlantis::invariantgraph {

BoolAllEqualNode::BoolAllEqualNode(VarNodeId a, VarNodeId b, VarNodeId r,
                                   bool breaksCycle)
    : BoolAllEqualNode(std::vector<VarNodeId>{a, b}, r, breaksCycle) {}

BoolAllEqualNode::BoolAllEqualNode(VarNodeId a, VarNodeId b, bool shouldHold,
                                   bool breaksCycle)
    : BoolAllEqualNode(std::vector<VarNodeId>{a, b}, shouldHold, breaksCycle) {}

BoolAllEqualNode::BoolAllEqualNode(std::vector<VarNodeId>&& vars, VarNodeId r,
                                   bool breaksCycle)
    : ViolationInvariantNode(std::move(vars), r), _breaksCycle(breaksCycle) {}

BoolAllEqualNode::BoolAllEqualNode(std::vector<VarNodeId>&& vars,
                                   bool shouldHold, bool breaksCycle)
    : ViolationInvariantNode(std::move(vars), shouldHold),
      _breaksCycle(breaksCycle) {}

void BoolAllEqualNode::updateState(InvariantGraph& graph) {
  std::vector<bool> isRemovable(staticInputVarNodeIds().size(), false);
  for (size_t i = 0; i < staticInputVarNodeIds().size() - 1; i++) {
    if (isRemovable[i]) {
      continue;
    }
    for (size_t j = i + 1; j < staticInputVarNodeIds().size(); j++) {
      if (staticInputVarNodeIds()[i] == staticInputVarNodeIds()[j]) {
        isRemovable[j] = true;
      }
    }
  }
  for (Int i = staticInputVarNodeIds().size(); i-- > 0;) {
    if (isRemovable[i]) {
      removeStaticInputVarNode(graph.varNode(staticInputVarNodeIds()[i]));
    }
  }
  if (staticInputVarNodeIds().size() <= 1) {
    if (isReified()) {
      graph.varNode(reifiedViolationNodeId()).fixToValue(true);
    }
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool BoolAllEqualNode::canBeReplaced(const InvariantGraph&) const {
  return staticInputVarNodeIds().size() <= 2;
}

bool BoolAllEqualNode::replace(InvariantGraph& graph) {
  if (!canBeReplaced(graph)) {
    return false;
  }
  if (staticInputVarNodeIds().size() == 2) {
    if (isReified()) {
      graph.addInvariantNode(std::make_unique<BoolEqNode>(
          staticInputVarNodeIds().front(), staticInputVarNodeIds().back(),
          reifiedViolationNodeId(), _breaksCycle));
    } else {
      graph.addInvariantNode(std::make_unique<BoolEqNode>(
          staticInputVarNodeIds().front(), staticInputVarNodeIds().back(),
          shouldHold(), _breaksCycle));
    }
  }
  return true;
}

void BoolAllEqualNode::registerOutputVars(InvariantGraph& invariantGraph,
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

void BoolAllEqualNode::registerNode(InvariantGraph& invariantGraph,
                                    propagation::SolverBase& solver) {
  if (staticInputVarNodeIds().empty()) {
    return;
  }
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);

  std::vector<propagation::VarId> solverVars;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars),
                 [&](const auto& id) { return invariantGraph.varId(id); });

  solver.makeViolationInvariant<propagation::BoolAllEqual>(
      solver, !shouldHold() ? _intermediate : violationVarId(invariantGraph),
      std::move(solverVars));
}

}  // namespace atlantis::invariantgraph
