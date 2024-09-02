#include "atlantis/invariantgraph/violationInvariantNodes/boolAndNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/boolAnd.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {

BoolAndNode::BoolAndNode(VarNodeId a, VarNodeId b, VarNodeId r)
    : ViolationInvariantNode(std::vector<VarNodeId>{a, b}, r) {}
BoolAndNode::BoolAndNode(VarNodeId a, VarNodeId b, bool shouldHold)
    : ViolationInvariantNode(std::vector<VarNodeId>{a, b}, shouldHold) {}

void BoolAndNode::registerOutputVars() {
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
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void BoolAndNode::init(const InvariantNodeId& id) {
  ViolationInvariantNode::init(id);
  assert(!isReified() ||
         !graph.varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(std::none_of(staticInputVarNodeIds().begin(),
                      staticInputVarNodeIds().end(), [&](const VarNodeId& vId) {
                        return graph.varNodeConst(vId).isIntVar();
                      }));
}

void BoolAndNode::registerNode() {
  assert(violationVarId(graph) != propagation::NULL_ID);
  assert(graph.varId(a()) != propagation::NULL_ID);
  assert(graph.varId(b()) != propagation::NULL_ID);

  solver.makeInvariant<propagation::BoolAnd>(
      solver, violationVarId(graph), graph.varId(a()), graph.varId(b()));
}

}  // namespace atlantis::invariantgraph
