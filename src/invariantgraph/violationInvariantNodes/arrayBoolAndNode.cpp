#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolAndNode.hpp"

#include "../parseHelper.hpp"
#include "atlantis/exceptions/exceptions.hpp"
#include "atlantis/invariantgraph/views/boolNotNode.hpp"
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

void ArrayBoolAndNode::init(InvariantGraph& graph, const InvariantNodeId& id) {
  ViolationInvariantNode::init(graph, id);
  assert(!isReified() ||
         !graph.varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(std::none_of(staticInputVarNodeIds().begin(),
                      staticInputVarNodeIds().end(), [&](const VarNodeId& vId) {
                        return graph.varNodeConst(vId).isIntVar();
                      }));
}

void ArrayBoolAndNode::updateState(InvariantGraph& graph) {
  ViolationInvariantNode::updateState(graph);
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());
  // remove fixed inputs that are true:
  for (const auto& id : staticInputVarNodeIds()) {
    if (graph.varNodeConst(id).isFixed()) {
      if (graph.varNodeConst(id).inDomain(bool{true})) {
        varsToRemove.emplace_back(id);
      } else if (shouldHold()) {
        throw InconsistencyException(
            "ArrayBoolAndNode::updateState constraint is violated");
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
    } else if (!shouldHold()) {
      throw InconsistencyException(
          "ArrayBoolAndNode::updateState constraint is violated");
    }
    setState(InvariantNodeState::SUBSUMED);
  } else if (staticInputVarNodeIds().size() == 1 && !isReified()) {
    auto& inputNode = graph.varNode(staticInputVarNodeIds().front());
    inputNode.fixToValue(shouldHold());
    removeStaticInputVarNode(inputNode);
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
  if (staticInputVarNodeIds().size() == 1) {
    if (isReified()) {
      graph.replaceVarNode(reifiedViolationNodeId(),
                           staticInputVarNodeIds().front());
    }
  }
  return true;
}

void ArrayBoolAndNode::registerOutputVars(InvariantGraph& graph,
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

void ArrayBoolAndNode::registerNode(InvariantGraph& graph,
                                    propagation::SolverBase& solver) {
  if (staticInputVarNodeIds().size() <= 1) {
    return;
  }
  assert(violationVarId(graph) != propagation::NULL_ID);
  std::vector<propagation::VarId> solverVars;
  solverVars.reserve(staticInputVarNodeIds().size());
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars),
                 [&](const auto& node) { return graph.varId(node); });
  if (solverVars.size() == 2) {
    solver.makeInvariant<propagation::BoolAnd>(
        solver, !shouldHold() ? _intermediate : violationVarId(graph),
        solverVars.front(), solverVars.back());
  } else {
    solver.makeInvariant<propagation::ForAll>(
        solver, !shouldHold() ? _intermediate : violationVarId(graph),
        std::move(solverVars));
  }
}

}  // namespace atlantis::invariantgraph
