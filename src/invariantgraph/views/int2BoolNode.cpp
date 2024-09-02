#include "atlantis/invariantgraph/views/int2BoolNode.hpp"

#include <map>
#include <utility>

#include "atlantis/propagation/views/int2BoolView.hpp"

namespace atlantis::invariantgraph {

Int2BoolNode::Int2BoolNode(IInvariantGraph& graph, VarNodeId staticInput,
                           VarNodeId output)
    : InvariantNode(graph, {output}, {staticInput}) {}

void Int2BoolNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(!invariantGraphConst()
              .varNodeConst(outputVarNodeIds().front())
              .isIntVar());
  assert(invariantGraph()
             .varNodeConst(staticInputVarNodeIds().front())
             .isIntVar());
}

void Int2BoolNode::updateState() {
  invariantGraph().varNode(input()).domain().removeBelow(Int{0});
  invariantGraph().varNode(input()).domain().removeAbove(Int{1});

  invariantGraph()
      .varNode(outputVarNodeIds().front())
      .domain()
      .removeBelow(Int{0});
  invariantGraph()
      .varNode(outputVarNodeIds().front())
      .domain()
      .removeAbove(Int{1});

  if (invariantGraphConst().varNodeConst(input()).isFixed()) {
    invariantGraph()
        .varNode(outputVarNodeIds().front())
        .fixToValue(
            invariantGraphConst().varNodeConst(input()).inDomain(Int{1}));
    setState(InvariantNodeState::SUBSUMED);
  } else if (invariantGraph()
                 .varNodeConst(outputVarNodeIds().front())
                 .isFixed()) {
    invariantGraph().varNode(input()).fixToValue(
        invariantGraph()
                .varNodeConst(outputVarNodeIds().front())
                .inDomain(bool{true})
            ? Int{1}
            : Int{0});
    setState(InvariantNodeState::SUBSUMED);
  }
}

void Int2BoolNode::registerOutputVars() {
  if (invariantGraph().varId(outputVarNodeIds().front()) ==
      propagation::NULL_ID) {
    invariantGraph()
        .varNode(outputVarNodeIds().front())
        .setVarId(solver().makeIntView<propagation::Int2BoolView>(
            solver(), invariantGraph().varId(input())));
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId vId) {
                       return invariantGraphConst().varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void Int2BoolNode::registerNode() {}

std::string Int2BoolNode::dotLangIdentifier() const { return "int2bool"; }

}  // namespace atlantis::invariantgraph
