#include "atlantis/invariantgraph/violationInvariantNodes/boolAndNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/boolAnd.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {

BoolAndNode::BoolAndNode(IInvariantGraph& graph, VarNodeId a, VarNodeId b,
                         VarNodeId r)
    : ViolationInvariantNode(graph, std::vector<VarNodeId>{a, b}, r) {}
BoolAndNode::BoolAndNode(IInvariantGraph& graph, VarNodeId a, VarNodeId b,
                         bool shouldHold)
    : ViolationInvariantNode(graph, std::vector<VarNodeId>{a, b}, shouldHold) {}

void BoolAndNode::registerOutputVars() {
  if (violationVarId() == propagation::NULL_ID) {
    if (shouldHold()) {
      registerViolation();
    } else {
      assert(!isReified());
      _intermediate = solver().makeIntVar(0, 0, 0);
      setViolationVarId(solver().makeIntView<propagation::NotEqualConst>(
          solver(), _intermediate, 0));
    }
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId vId) {
                       return invariantGraphConst().varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void BoolAndNode::init(InvariantNodeId id) {
  ViolationInvariantNode::init(id);
  assert(
      !isReified() ||
      !invariantGraphConst().varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(
      std::none_of(staticInputVarNodeIds().begin(),
                   staticInputVarNodeIds().end(), [&](const VarNodeId vId) {
                     return invariantGraphConst().varNodeConst(vId).isIntVar();
                   }));
}

void BoolAndNode::registerNode() {
  assert(violationVarId() != propagation::NULL_ID);
  assert(violationVarId().isVar());

  assert(invariantGraph().varId(a()) != propagation::NULL_ID);
  assert(invariantGraph().varId(b()) != propagation::NULL_ID);

  solver().makeInvariant<propagation::BoolAnd>(solver(), violationVarId(),
                                               invariantGraph().varId(a()),
                                               invariantGraph().varId(b()));
}

std::string BoolAndNode::dotLangIdentifier() const { return "bool_and"; }

}  // namespace atlantis::invariantgraph
