#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolXorNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolXorNode.hpp"
#include "atlantis/propagation/invariants/boolLinear.hpp"
#include "atlantis/propagation/views/equalConst.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {

ArrayBoolXorNode::ArrayBoolXorNode(std::vector<VarNodeId>&& as,
                                   VarNodeId output)
    : ViolationInvariantNode(std::move(as), output) {}

ArrayBoolXorNode::ArrayBoolXorNode(std::vector<VarNodeId>&& as, bool shouldHold)
    : ViolationInvariantNode(std::move(as), shouldHold) {}

void ArrayBoolXorNode::updateState(InvariantGraph& invariantGraph) {
  ViolationInvariantNode::updateState(invariantGraph);
  if (isReified() || !shouldHold()) {
    return;
  }
  size_t numTrue = 0;
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());
  for (const auto& id : staticInputVarNodeIds()) {
    if (invariantGraph.varNodeConst(id).isFixed()) {
      if (invariantGraph.varNodeConst(id).inDomain(true)) {
        numTrue++;
      } else {
        varsToRemove.push_back(id);
      }
    }
  }
  for (const auto& id : varsToRemove) {
    removeStaticInputVarNode(invariantGraph.varNode(id));
  }
  if (staticInputVarNodeIds().size() == 0 || numTrue > 1) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool ArrayBoolXorNode::canBeReplaced(const InvariantGraph&) const {
  if (state() != InvariantNodeState::ACTIVE || isReified() || !shouldHold()) {
    return false;
  }
  return staticInputVarNodeIds().size() <= 2;
}

bool ArrayBoolXorNode::replace(InvariantGraph& graph) {
  if (!canBeReplaced(graph)) {
    return false;
  }
  assert(staticInputVarNodeIds().size() <= 2);
  if (staticInputVarNodeIds().size() == 1) {
    graph.varNode(staticInputVarNodeIds().front()).setIsViolationVar(true);
  } else if (staticInputVarNodeIds().size() == 2) {
    graph.addInvariantNode(std::make_unique<BoolXorNode>(
        staticInputVarNodeIds().front(), staticInputVarNodeIds().back(), true));
  }
  return true;
}

void ArrayBoolXorNode::registerOutputVars(InvariantGraph& invariantGraph,
                                          propagation::SolverBase& solver) {
  if (violationVarId(invariantGraph) == propagation::NULL_ID) {
    _intermediate = solver.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(invariantGraph,
                        solver.makeIntView<propagation::EqualConst>(
                            solver, _intermediate, 1));
    } else {
      assert(!isReified());
      setViolationVarId(invariantGraph,
                        solver.makeIntView<propagation::NotEqualConst>(
                            solver, _intermediate, 1));
    }
  }
}

void ArrayBoolXorNode::registerNode(InvariantGraph& invariantGraph,
                                    propagation::SolverBase& solver) {
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);
  std::vector<propagation::VarId> inputNodeIds;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(inputNodeIds),
                 [&](const auto& node) { return invariantGraph.varId(node); });

  solver.makeInvariant<propagation::BoolLinear>(solver, _intermediate,
                                                std::move(inputNodeIds));
}

}  // namespace atlantis::invariantgraph
