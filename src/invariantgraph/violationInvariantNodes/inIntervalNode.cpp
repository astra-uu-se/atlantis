#include "atlantis/invariantgraph/violationInvariantNodes/inIntervalNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/views/inIntervalConst.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/utils/variant.hpp"

namespace atlantis::invariantgraph {

InIntervalNode::InIntervalNode(IInvariantGraph& graph, VarNodeId input, Int lb,
                               Int ub, VarNodeId r)
    : ViolationInvariantNode(graph, {input}, r), _lb(lb), _ub(ub) {}

InIntervalNode::InIntervalNode(IInvariantGraph& graph, VarNodeId input, Int lb,
                               Int ub, bool shouldHold)
    : ViolationInvariantNode(graph, {input}, shouldHold), _lb(lb), _ub(ub) {}

void InIntervalNode::init(InvariantNodeId id) {
  ViolationInvariantNode::init(id);
  assert(
      !isReified() ||
      !invariantGraphConst().varNodeConst(reifiedViolationNodeId()).isIntVar());
  assert(
      std::all_of(staticInputVarNodeIds().begin(),
                  staticInputVarNodeIds().end(), [&](const VarNodeId vId) {
                    return invariantGraphConst().varNodeConst(vId).isIntVar();
                  }));
}

void InIntervalNode::registerOutputVars() {
  if (violationVarId() == propagation::NULL_ID) {
    if (shouldHold()) {
      setViolationVarId(solver().makeIntView<propagation::InIntervalConst>(
          solver(), invariantGraph().varId(staticInputVarNodeIds().front()),
          _lb, _ub));
    } else {
      assert(!isReified());
      _intermediate = solver().makeIntView<propagation::InIntervalConst>(
          solver(), invariantGraph().varId(staticInputVarNodeIds().front()),
          _lb, _ub);
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

void InIntervalNode::registerNode() {}

std::string InIntervalNode::dotLangIdentifier() const { return "in_interval"; }

}  // namespace atlantis::invariantgraph
