#include "atlantis/invariantgraph/views/boolNotNode.hpp"

#include "atlantis/propagation/views/bool2IntView.hpp"

namespace atlantis::invariantgraph {

BoolNotNode::BoolNotNode(IInvariantGraph& invariantGraph, VarNodeId staticInput,
                         VarNodeId output)
    : InvariantNode(invariantGraph, {output}, {staticInput}) {}

void BoolNotNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(!invariantGraphConst()
              .varNodeConst(outputVarNodeIds().front())
              .isIntVar());
  assert(!invariantGraph()
              .varNodeConst(staticInputVarNodeIds().front())
              .isIntVar());
}

void BoolNotNode::updateState() {
  if (invariantGraphConst().varNodeConst(input()).isFixed()) {
    invariantGraph()
        .varNode(outputVarNodeIds().front())
        .fixToValue(
            !invariantGraphConst().varNodeConst(input()).inDomain(bool{true}));
    setState(InvariantNodeState::SUBSUMED);
  } else if (invariantGraph()
                 .varNodeConst(outputVarNodeIds().front())
                 .isFixed()) {
    invariantGraph().varNode(input()).fixToValue(
        !invariantGraph()
             .varNodeConst(outputVarNodeIds().front())
             .inDomain(bool{true}));
    setState(InvariantNodeState::SUBSUMED);
  }
}

void BoolNotNode::registerOutputVars() {
  if (invariantGraph().varId(outputVarNodeIds().front()) ==
      propagation::NULL_ID) {
    invariantGraph()
        .varNode(outputVarNodeIds().front())
        .setVarId(solver().makeIntView<propagation::Bool2IntView>(
            solver(), invariantGraph().varId(input())));
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId vId) {
                       return invariantGraphConst().varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void BoolNotNode::registerNode() {}

std::string BoolNotNode::dotLangIdentifier() const { return "bool_not"; }

}  // namespace atlantis::invariantgraph
