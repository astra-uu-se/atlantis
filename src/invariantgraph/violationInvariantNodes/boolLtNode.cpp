#include "atlantis/invariantgraph/violationInvariantNodes/boolLtNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/views/boolNotNode.hpp"
#include "atlantis/propagation/violationInvariants/boolLessEqual.hpp"
#include "atlantis/propagation/violationInvariants/boolLessThan.hpp"

namespace atlantis::invariantgraph {

BoolLtNode::BoolLtNode(VarNodeId a, VarNodeId b, VarNodeId r)
    : ViolationInvariantNode(std::vector<VarNodeId>{a, b}, r) {}

BoolLtNode::BoolLtNode(VarNodeId a, VarNodeId b, bool shouldHold)
    : ViolationInvariantNode(std::vector<VarNodeId>{a, b}, shouldHold) {}

void BoolLtNode::init(InvariantGraph& graph, const InvariantNodeId& id) {
  ViolationInvariantNode::init(graph, id);
  assert(!isReified() ||
         !graph.varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(std::none_of(staticInputVarNodeIds().begin(),
                      staticInputVarNodeIds().end(), [&](const VarNodeId& vId) {
                        return graph.varNodeConst(vId).isIntVar();
                      }));
}

void BoolLtNode::updateState(InvariantGraph& graph) {
  ViolationInvariantNode::updateState(graph);
  if (staticInputVarNodeIds().size() < 2) {
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  VarNode& aNode = graph.varNode(a());
  VarNode& bNode = graph.varNode(b());
  if (a() == b()) {
    if (isReified()) {
      fixReified(graph, false);
    } else if (shouldHold()) {
      throw InconsistencyException("BoolLtNode: a == b");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (!isReified() && shouldHold()) {
    aNode.fixToValue(bool{false});
    bNode.fixToValue(bool{true});
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (aNode.isFixed() && bNode.isFixed()) {
    const bool isSatisifed =
        aNode.inDomain(bool{false}) && bNode.inDomain(bool{true});
    if (isReified()) {
      fixReified(graph, isSatisifed);
    } else if (isSatisifed != shouldHold()) {
      throw InconsistencyException(shouldHold() ? "BoolLtNode: a >= b"
                                                : "BoolLtNode neg: a < b");
    }
    setState(InvariantNodeState::SUBSUMED);
    return;
  } else if (aNode.isFixed() || bNode.isFixed()) {
    assert(aNode.isFixed() != bNode.isFixed());
    const bool isViolated = (aNode.isFixed() && aNode.inDomain(bool{true})) ||
                            (bNode.isFixed() && bNode.inDomain(bool{false}));
    if (isViolated) {
      if (isReified()) {
        fixReified(graph, !isViolated);
      } else if (shouldHold()) {
        throw InconsistencyException("BoolLtNode: a >= b");
      }
      setState(InvariantNodeState::SUBSUMED);
    }
  }
}

bool BoolLtNode::canBeReplaced(const InvariantGraph& graph) const {
  return state() == InvariantNodeState::ACTIVE &&
         staticInputVarNodeIds().size() == 2 &&
         (isReified() && graph.varNodeConst(a()).isFixed() !=
                             graph.varNodeConst(b()).isFixed());
}

bool BoolLtNode::replace(InvariantGraph& graph) {
  if (!canBeReplaced(graph)) {
    return false;
  }
  assert(isReified());
  if (graph.varNode(a()).isFixed()) {
    assert(graph.varNode(a()).inDomain(bool{false}));
    graph.replaceVarNode(reifiedViolationNodeId(), b());
  } else {
    assert(graph.varNode(b()).isFixed() &&
           graph.varNode(b()).inDomain(bool{true}));
    graph.addInvariantNode(
        std::make_unique<BoolNotNode>(a(), reifiedViolationNodeId()));
  }
  return true;
}

void BoolLtNode::registerOutputVars(InvariantGraph& graph,
                                    propagation::SolverBase& solver) {
  registerViolation(graph, solver);
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void BoolLtNode::registerNode(InvariantGraph& graph,
                              propagation::SolverBase& solver) {
  assert(violationVarId(graph) != propagation::NULL_ID);
  assert(graph.varId(a()) != propagation::NULL_ID);
  assert(graph.varId(b()) != propagation::NULL_ID);

  if (shouldHold()) {
    solver.makeViolationInvariant<propagation::BoolLessThan>(
        solver, violationVarId(graph), graph.varId(a()), graph.varId(b()));
  } else {
    assert(!isReified());
    solver.makeViolationInvariant<propagation::BoolLessEqual>(
        solver, violationVarId(graph), graph.varId(b()), graph.varId(a()));
  }
}

std::string BoolLtNode::dotLangIdentifier() const { return "bool_lt"; }

}  // namespace atlantis::invariantgraph
