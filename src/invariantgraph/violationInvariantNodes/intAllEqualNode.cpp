#include "atlantis/invariantgraph/violationInvariantNodes/intAllEqualNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/allDifferentNode.hpp"
#include "atlantis/propagation/views/equalConst.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/propagation/violationInvariants/allDifferent.hpp"
#include "atlantis/propagation/violationInvariants/equal.hpp"

namespace atlantis::invariantgraph {

IntAllEqualNode::IntAllEqualNode(VarNodeId a, VarNodeId b, VarNodeId r,
                                 bool breaksCycle)
    : IntAllEqualNode(std::vector<VarNodeId>{a, b}, r, breaksCycle) {}

IntAllEqualNode::IntAllEqualNode(VarNodeId a, VarNodeId b, bool shouldHold,
                                 bool breaksCycle)
    : IntAllEqualNode(std::vector<VarNodeId>{a, b}, shouldHold, breaksCycle) {}

IntAllEqualNode::IntAllEqualNode(std::vector<VarNodeId>&& vars, VarNodeId r,
                                 bool breaksCycle)
    : ViolationInvariantNode(std::move(vars), r), _breaksCycle(breaksCycle) {}

IntAllEqualNode::IntAllEqualNode(std::vector<VarNodeId>&& vars, bool shouldHold,
                                 bool breaksCycle)
    : ViolationInvariantNode(std::move(vars), shouldHold),
      _breaksCycle(breaksCycle) {}

void IntAllEqualNode::updateState(InvariantGraph& graph) {
  ViolationInvariantNode::updateState(graph);
  if (staticInputVarNodeIds().size() < 2) {
    if (!shouldHold()) {
      throw InconsistencyException(
          "IntAllEqualNode::updateState constraint is violated");
    }
    if (isReified()) {
      graph.varNode(reifiedViolationNodeId()).fixToValue(bool{true});
    }
    setState(InvariantNodeState::SUBSUMED);
  }
  size_t numFixed = 0;
  for (size_t i = 0; i < staticInputVarNodeIds().size(); ++i) {
    const VarNode& iNode = graph.varNodeConst(staticInputVarNodeIds().at(i));
    if (!iNode.isFixed()) {
      continue;
    }
    ++numFixed;
    const Int iVal = iNode.lowerBound();
    for (size_t j = i + 1; j < staticInputVarNodeIds().size(); ++j) {
      const VarNode& jNode = graph.varNodeConst(staticInputVarNodeIds().at(j));
      if (!jNode.isFixed()) {
        continue;
      }
      const Int jVal = jNode.lowerBound();
      if (iVal != jVal) {
        if (isReified()) {
          graph.varNode(reifiedViolationNodeId()).fixToValue(bool{false});
        } else if (shouldHold()) {
          throw InconsistencyException(
              "IntAllEqualNode::updateState constraint is violated");
        }
      }
    }
  }
  if (numFixed == staticInputVarNodeIds().size()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool IntAllEqualNode::canBeReplaced(const InvariantGraph&) const {
  return state() == InvariantNodeState::ACTIVE && !_breaksCycle &&
         (!isReified() &&
          (shouldHold() || staticInputVarNodeIds().size() <= 2));
}

bool IntAllEqualNode::replace(InvariantGraph& invariantGraph) {
  if (!canBeReplaced(invariantGraph)) {
    return false;
  }
  if (!shouldHold()) {
    assert(staticInputVarNodeIds().size() == 2);
    invariantGraph.addInvariantNode(std::make_unique<AllDifferentNode>(
        std::vector<VarNodeId>{staticInputVarNodeIds()}, true));
  } else if (!staticInputVarNodeIds().empty()) {
    const VarNodeId firstVar = staticInputVarNodeIds().front();
    for (size_t i = 1; i < staticInputVarNodeIds().size(); ++i) {
      invariantGraph.replaceVarNode(staticInputVarNodeIds().at(i), firstVar);
    }
  }
  return true;
}

void IntAllEqualNode::registerOutputVars(InvariantGraph& invariantGraph,
                                         propagation::SolverBase& solver) {
  if (violationVarId(invariantGraph) == propagation::NULL_ID) {
    _allDifferentViolationVarId = solver.makeIntVar(0, 0, 0);
    if (shouldHold()) {
      setViolationVarId(invariantGraph,
                        solver.makeIntView<propagation::EqualConst>(
                            solver, _allDifferentViolationVarId,
                            staticInputVarNodeIds().size() - 1));
    } else {
      assert(!isReified());
      setViolationVarId(invariantGraph,
                        solver.makeIntView<propagation::NotEqualConst>(
                            solver, _allDifferentViolationVarId,
                            staticInputVarNodeIds().size() - 1));
    }
  }
}

void IntAllEqualNode::registerNode(InvariantGraph& invariantGraph,
                                   propagation::SolverBase& solver) {
  assert(_allDifferentViolationVarId != propagation::NULL_ID);
  assert(violationVarId(invariantGraph) != propagation::NULL_ID);

  std::vector<propagation::VarId> inputVarIds;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(inputVarIds),
                 [&](const auto& id) { return invariantGraph.varId(id); });

  solver.makeViolationInvariant<propagation::AllDifferent>(
      solver, _allDifferentViolationVarId, std::move(inputVarIds));
}

}  // namespace atlantis::invariantgraph
