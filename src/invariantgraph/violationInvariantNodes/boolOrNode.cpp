#include "atlantis/invariantgraph/violationInvariantNodes/boolOrNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/boolOr.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"

namespace atlantis::invariantgraph {

BoolOrNode::BoolOrNode(IInvariantGraph& graph, VarNodeId a, VarNodeId b,
                       VarNodeId r)
    : ViolationInvariantNode(graph, std::vector<VarNodeId>{a, b}, r) {}

BoolOrNode::BoolOrNode(IInvariantGraph& graph, VarNodeId a, VarNodeId b,
                       bool shouldHold)
    : ViolationInvariantNode(graph, std::vector<VarNodeId>{a, b}, shouldHold) {}

void BoolOrNode::init(InvariantNodeId id) {
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

void BoolOrNode::registerOutputVars() {
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

void BoolOrNode::registerNode() {
  assert(violationVarId() != propagation::NULL_ID);
  assert(violationVarId().isVar());

  solver().makeInvariant<propagation::BoolOr>(solver(), violationVarId(),
                                              invariantGraph().varId(a()),
                                              invariantGraph().varId(b()));
}

std::string BoolOrNode::dotLangIdentifier() const { return "bool_or"; }

}  // namespace atlantis::invariantgraph
