#include "atlantis/invariantgraph/violationInvariantNodes/intAllEqualNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/allDifferentNode.hpp"
#include "atlantis/propagation/views/equalConst.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/propagation/violationInvariants/allDifferent.hpp"
#include "atlantis/propagation/violationInvariants/equal.hpp"
#include "atlantis/propagation/violationInvariants/notEqual.hpp"

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

void IntAllEqualNode::init(InvariantGraph& graph, const InvariantNodeId& id) {
  ViolationInvariantNode::init(graph, id);
  assert(!isReified() ||
         !graph.varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(std::all_of(staticInputVarNodeIds().begin(),
                     staticInputVarNodeIds().end(), [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).isIntVar();
                     }));
}

void IntAllEqualNode::updateState(InvariantGraph& graph) {
  ViolationInvariantNode::updateState(graph);
  if (staticInputVarNodeIds().size() < 2) {
    if (!shouldHold()) {
      throw InconsistencyException(
          "IntAllEqualNode::updateState constraint is violated");
    }
    if (isReified()) {
      fixReified(graph, true);
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
          fixReified(graph, false);
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

void IntAllEqualNode::registerOutputVars(InvariantGraph& graph,
                                         propagation::SolverBase& solver) {
  assert(staticInputVarNodeIds().size() >= 2);
  if (violationVarId(graph) == propagation::NULL_ID) {
    if (staticInputVarNodeIds().size() == 2) {
      registerViolation(graph, solver);
    } else if (_allDifferentViolationVarId == propagation::NULL_ID) {
      _allDifferentViolationVarId = solver.makeIntVar(0, 0, 0);
      if (shouldHold()) {
        setViolationVarId(graph, solver.makeIntView<propagation::EqualConst>(
                                     solver, _allDifferentViolationVarId,
                                     staticInputVarNodeIds().size() - 1));
      } else {
        assert(!isReified());
        setViolationVarId(graph, solver.makeIntView<propagation::NotEqualConst>(
                                     solver, _allDifferentViolationVarId,
                                     staticInputVarNodeIds().size() - 1));
      }
    }
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void IntAllEqualNode::registerNode(InvariantGraph& graph,
                                   propagation::SolverBase& solver) {
  assert(staticInputVarNodeIds().size() >= 2);
  assert(violationVarId(graph) != propagation::NULL_ID);

  std::vector<propagation::VarId> inputVarIds;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(inputVarIds),
                 [&](const auto& id) { return graph.varId(id); });

  if (inputVarIds.size() == 2) {
    if (shouldHold()) {
      solver.makeViolationInvariant<propagation::Equal>(
          solver, violationVarId(graph), inputVarIds.front(),
          inputVarIds.back());
    } else {
      solver.makeViolationInvariant<propagation::NotEqual>(
          solver, violationVarId(graph), inputVarIds.front(),
          inputVarIds.back());
    }
    return;
  }

  assert(_allDifferentViolationVarId != propagation::NULL_ID);

  solver.makeViolationInvariant<propagation::AllDifferent>(
      solver, _allDifferentViolationVarId, std::move(inputVarIds));
}

}  // namespace atlantis::invariantgraph
