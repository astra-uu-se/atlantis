#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolOrNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/views/boolNotNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolEqNode.hpp"
#include "atlantis/propagation/invariants/boolOr.hpp"
#include "atlantis/propagation/invariants/exists.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {

ArrayBoolOrNode::ArrayBoolOrNode(VarNodeId a, VarNodeId b, VarNodeId output)
    : ViolationInvariantNode(std::vector<VarNodeId>{a, b}, output) {}

ArrayBoolOrNode::ArrayBoolOrNode(VarNodeId a, VarNodeId b, bool shouldHold)
    : ViolationInvariantNode(std::vector<VarNodeId>{a, b}, shouldHold) {}

ArrayBoolOrNode::ArrayBoolOrNode(std::vector<VarNodeId>&& inputs,
                                 VarNodeId output)
    : ViolationInvariantNode(std::move(inputs), output) {}

ArrayBoolOrNode::ArrayBoolOrNode(std::vector<VarNodeId>&& inputs,
                                 bool shouldHold)
    : ViolationInvariantNode(std::move(inputs), shouldHold) {}

void ArrayBoolOrNode::updateState(InvariantGraph& graph) {
  ViolationInvariantNode::updateState(graph);
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());
  // remove fixed inputs that are false:
  for (const auto& id : staticInputVarNodeIds()) {
    if (graph.varNodeConst(id).isFixed()) {
      if (graph.varNodeConst(id).inDomain(bool{false})) {
        varsToRemove.emplace_back(id);
      } else if (!shouldHold()) {
        throw InconsistencyException(
            "ArrayBoolOrNode::updateState constraint is violated");
      } else {
        setState(InvariantNodeState::SUBSUMED);
        return;
      }
    }
  }

  for (const auto& id : varsToRemove) {
    removeStaticInputVarNode(graph.varNode(id));
  }

  if (staticInputVarNodeIds().size() == 0) {
    if (isReified()) {
      graph.varNode(reifiedViolationNodeId()).fixToValue(true);
    } else if (shouldHold()) {
      throw InconsistencyException(
          "ArrayBoolOrNode::updateState constraint is violated");
    }
    setState(InvariantNodeState::SUBSUMED);
  } else if (staticInputVarNodeIds().size() == 1 && !isReified()) {
    auto& inputNode = graph.varNode(staticInputVarNodeIds().front());
    inputNode.fixToValue(shouldHold());
    removeStaticInputVarNode(inputNode);
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool ArrayBoolOrNode::canBeReplaced(const InvariantGraph&) const {
  return state() == InvariantNodeState::ACTIVE &&
         staticInputVarNodeIds().size() <= 1;
}

bool ArrayBoolOrNode::replace(InvariantGraph& graph) {
  if (!canBeReplaced(graph)) {
    return false;
  }
  if (staticInputVarNodeIds().size() == 1) {
    if (isReified()) {
      graph.replaceVarNode(reifiedViolationNodeId(),
                           staticInputVarNodeIds().front());
    }
  }
  return true;
}

void ArrayBoolOrNode::registerOutputVars(InvariantGraph& graph,
                                         propagation::SolverBase& solver) {
  if (staticInputVarNodeIds().size() <= 1) {
    return;
  }
  if (violationVarId(graph) == propagation::NULL_ID) {
    if (shouldHold()) {
      registerViolation(graph, solver);
    } else {
      assert(!isReified());
      _intermediate = solver.makeIntVar(0, 0, 0);
      setViolationVarId(graph, solver.makeIntView<propagation::NotEqualConst>(
                                   solver, _intermediate, 0));
    }
  }
}

void ArrayBoolOrNode::registerNode(InvariantGraph& graph,
                                   propagation::SolverBase& solver) {
  if (staticInputVarNodeIds().size() <= 1) {
    return;
  }
  assert(violationVarId(graph) != propagation::NULL_ID);
  std::vector<propagation::VarId> solverVars;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars),
                 [&](const auto& node) { return graph.varId(node); });

  if (solverVars.size() == 2) {
    solver.makeInvariant<propagation::BoolOr>(
        solver, !shouldHold() ? _intermediate : violationVarId(graph),
        solverVars.front(), solverVars.back());
  } else {
    solver.makeInvariant<propagation::Exists>(
        solver, !shouldHold() ? _intermediate : violationVarId(graph),
        std::move(solverVars));
  }
}

}  // namespace atlantis::invariantgraph
