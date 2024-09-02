#include "atlantis/invariantgraph/violationInvariantNodes/boolAllEqualNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/arrayBoolXorNode.hpp"
#include "atlantis/propagation/invariants/boolXor.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/propagation/violationInvariants/boolAllEqual.hpp"
#include "atlantis/propagation/violationInvariants/boolEqual.hpp"

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

void BoolAllEqualNode::init(InvariantGraph& graph, const InvariantNodeId& id) {
  ViolationInvariantNode::init(graph, id);
  assert(!isReified() ||
         !graph.varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(std::none_of(staticInputVarNodeIds().begin(),
                      staticInputVarNodeIds().end(), [&](const VarNodeId& vId) {
                        return graph.varNodeConst(vId).isIntVar();
                      }));
}

void BoolAllEqualNode::updateState(InvariantGraph& graph) {
  ViolationInvariantNode::updateState(graph);
  if (staticInputVarNodeIds().size() < 2) {
    if (isReified()) {
      fixReified(graph, true);
    } else if (!shouldHold()) {
      throw InconsistencyException(
          "BoolAllEqualNode::updateState constraint is violated");
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
    const bool iVal = iNode.inDomain(bool{true});
    for (size_t j = i + 1; j < staticInputVarNodeIds().size(); ++j) {
      const VarNode& jNode = graph.varNodeConst(staticInputVarNodeIds().at(j));
      if (!jNode.isFixed()) {
        continue;
      }
      const bool jVal = jNode.inDomain(bool{true});
      if (iVal != jVal) {
        if (isReified()) {
          fixReified(graph, false);
        } else if (shouldHold()) {
          throw InconsistencyException(
              "BoolAllEqualNode::updateState constraint is violated");
        }
      }
    }
  }
  if (numFixed == staticInputVarNodeIds().size()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool BoolAllEqualNode::canBeReplaced(const InvariantGraph&) const {
  return state() == InvariantNodeState::ACTIVE && !_breaksCycle &&
         (!isReified() &&
          (shouldHold() || staticInputVarNodeIds().size() <= 2));
}

bool BoolAllEqualNode::replace(InvariantGraph& graph) {
  if (!canBeReplaced(graph)) {
    return false;
  }
  if (!shouldHold()) {
    assert(staticInputVarNodeIds().size() == 2);
    graph.addInvariantNode(std::make_unique<ArrayBoolXorNode>(
        std::vector<VarNodeId>{staticInputVarNodeIds()}, true));
  } else if (!staticInputVarNodeIds().empty()) {
    const VarNodeId firstVar = staticInputVarNodeIds().front();
    for (size_t i = 1; i < staticInputVarNodeIds().size(); ++i) {
      graph.replaceVarNode(staticInputVarNodeIds().at(i), firstVar);
    }
  }
  return true;
}

void BoolAllEqualNode::registerOutputVars(InvariantGraph& graph,
                                          propagation::SolverBase& solver) {
  if (violationVarId(graph) == propagation::NULL_ID) {
    if (shouldHold() || staticInputVarNodeIds().size() == 2) {
      registerViolation(graph, solver);
    } else if (!shouldHold()) {
      assert(!isReified());
      _intermediate = solver.makeIntVar(0, 0, 0);
      setViolationVarId(graph, solver.makeIntView<propagation::NotEqualConst>(
                                   solver, _intermediate, 0));
    }
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void BoolAllEqualNode::registerNode(InvariantGraph& graph,
                                    propagation::SolverBase& solver) {
  if (staticInputVarNodeIds().empty()) {
    return;
  }
  assert(violationVarId(graph) != propagation::NULL_ID);

  std::vector<propagation::VarId> solverVars;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars),
                 [&](const auto& id) { return graph.varId(id); });

  if (solverVars.size() == 2) {
    if (shouldHold()) {
      solver.makeViolationInvariant<propagation::BoolEqual>(
          solver, violationVarId(graph), solverVars.front(), solverVars.back());

    } else {
      solver.makeInvariant<propagation::BoolXor>(
          solver, violationVarId(graph), solverVars.front(), solverVars.back());
    }
    return;
  }

  assert(shouldHold() || _intermediate != propagation::NULL_ID);

  solver.makeViolationInvariant<propagation::BoolAllEqual>(
      solver, !shouldHold() ? _intermediate : violationVarId(graph),
      std::move(solverVars));
}

std::string BoolAllEqualNode::dotLangIdentifier() const {
  return "bool_all_equal";
}

}  // namespace atlantis::invariantgraph
