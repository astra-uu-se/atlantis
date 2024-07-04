#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolXorNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/boolAllEqualNode.hpp"
#include "atlantis/propagation/invariants/boolLinear.hpp"
#include "atlantis/propagation/invariants/boolXor.hpp"
#include "atlantis/propagation/views/equalConst.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {

ArrayBoolXorNode::ArrayBoolXorNode(VarNodeId a, VarNodeId b, VarNodeId reified)
    : ArrayBoolXorNode(std::vector<VarNodeId>{a, b}, reified) {}

ArrayBoolXorNode::ArrayBoolXorNode(VarNodeId a, VarNodeId b, bool shouldHold)
    : ArrayBoolXorNode(std::vector<VarNodeId>{a, b}, shouldHold) {}

ArrayBoolXorNode::ArrayBoolXorNode(std::vector<VarNodeId>&& as,
                                   VarNodeId reified)
    : ViolationInvariantNode(std::move(as), reified) {}

ArrayBoolXorNode::ArrayBoolXorNode(std::vector<VarNodeId>&& as, bool shouldHold)
    : ViolationInvariantNode(std::move(as), shouldHold) {}

void ArrayBoolXorNode::updateState(InvariantGraph& graph) {
  ViolationInvariantNode::updateState(graph);
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());
  // keep track of index of fixed input that is true:
  size_t trueIndex = staticInputVarNodeIds().size();
  // remove fixed inputs that are false:
  for (size_t i = 0; i < staticInputVarNodeIds().size(); ++i) {
    VarNode& iNode = graph.varNode(staticInputVarNodeIds().at(i));
    if (!iNode.isFixed()) {
      continue;
    }
    if (iNode.inDomain(bool{false})) {
      varsToRemove.emplace_back(staticInputVarNodeIds().at(i));
    } else if (trueIndex >= staticInputVarNodeIds().size()) {
      trueIndex = i;
    } else {
      // Two or more inputs are fixed to true
      if (isReified()) {
        graph.varNode(reifiedViolationNodeId()).fixToValue(bool{false});
        // this violation invariant is no longer reified:
        ViolationInvariantNode::updateState(graph);
        assert(!isReified() && !shouldHold());
      } else if (shouldHold()) {
        throw InconsistencyException(
            "ArrayBoolOrNode::updateState constraint is violated");
        setState(InvariantNodeState::SUBSUMED);
        return;
      }
    }
  }
  if (trueIndex < staticInputVarNodeIds().size() && !isReified() &&
      shouldHold()) {
    // fix all inputs beside the true input to false:
    for (size_t i = 0; i < staticInputVarNodeIds().size(); ++i) {
      if (i == trueIndex) {
        continue;
      }
      graph.varNode(staticInputVarNodeIds().at(i)).fixToValue(bool{false});
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }

  for (const auto& id : varsToRemove) {
    removeStaticInputVarNode(graph.varNode(id));
  }

  if (staticInputVarNodeIds().empty()) {
    // array_bool_xor([]) == false
    if (isReified()) {
      graph.varNode(reifiedViolationNodeId()).fixToValue(bool{false});
    } else if (shouldHold()) {
      throw InconsistencyException(
          "ArrayBoolOrNode::updateState constraint is violated");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  } else if (staticInputVarNodeIds().size() == 1 && !isReified()) {
    graph.varNode(staticInputVarNodeIds().front()).fixToValue(shouldHold());
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool ArrayBoolXorNode::canBeReplaced(const InvariantGraph&) const {
  return state() == InvariantNodeState::ACTIVE &&
         ((isReified() && staticInputVarNodeIds().size() <= 1) ||
          (!isReified() && !shouldHold() &&
           staticInputVarNodeIds().size() == 2));
}

bool ArrayBoolXorNode::replace(InvariantGraph& graph) {
  if (!canBeReplaced(graph)) {
    return false;
  }
  if (staticInputVarNodeIds().size() == 1) {
    assert(isReified());
    graph.replaceVarNode(reifiedViolationNodeId(),
                         staticInputVarNodeIds().front());
  }
  if (staticInputVarNodeIds().size() == 2) {
    assert(!isReified() && !shouldHold());
    graph.addInvariantNode(std::make_unique<BoolAllEqualNode>(
        std::vector<VarNodeId>{staticInputVarNodeIds()}, true));
  }
  return true;
}

void ArrayBoolXorNode::registerOutputVars(InvariantGraph& graph,
                                          propagation::SolverBase& solver) {
  if (staticInputVarNodeIds().size() <= 1 ||
      violationVarId(graph) != propagation::NULL_ID) {
    return;
  }

  if (staticInputVarNodeIds().size() == 2) {
    assert(isReified() || shouldHold());
    registerViolation(graph, solver);
    return;
  }
  _intermediate = solver.makeIntVar(0, 0, 0);
  if (shouldHold()) {
    setViolationVarId(graph, solver.makeIntView<propagation::EqualConst>(
                                 solver, _intermediate, 1));
  } else {
    assert(!isReified() && staticInputVarNodeIds().size() > 2);
    setViolationVarId(graph, solver.makeIntView<propagation::NotEqualConst>(
                                 solver, _intermediate, 1));
  }
}

void ArrayBoolXorNode::registerNode(InvariantGraph& graph,
                                    propagation::SolverBase& solver) {
  if (staticInputVarNodeIds().size() <= 1) {
    return;
  }
  assert(violationVarId(graph) != propagation::NULL_ID);
  std::vector<propagation::VarId> inputNodeIds;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(inputNodeIds),
                 [&](const auto& node) { return graph.varId(node); });
  if (staticInputVarNodeIds().size() == 2) {
    assert(isReified() || shouldHold());
    assert(_intermediate == propagation::NULL_ID);
    solver.makeInvariant<propagation::BoolXor>(solver, violationVarId(graph),
                                               inputNodeIds.front(),
                                               inputNodeIds.back());
    return;
  }

  solver.makeInvariant<propagation::BoolLinear>(solver, _intermediate,
                                                std::move(inputNodeIds));
}

}  // namespace atlantis::invariantgraph
