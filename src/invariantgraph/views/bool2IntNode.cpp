#include "atlantis/invariantgraph/views/bool2IntNode.hpp"

#include <map>
#include <utility>

#include "atlantis/propagation/views/bool2IntView.hpp"

namespace atlantis::invariantgraph {

Bool2IntNode::Bool2IntNode(IInvariantGraph& graph, VarNodeId staticInput,
                           VarNodeId output)
    : InvariantNode(graph, {output}, {staticInput}) {}

void Bool2IntNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(invariantGraphConst()
             .varNodeConst(outputVarNodeIds().front())
             .isIntVar());
  assert(!invariantGraph()
              .varNodeConst(staticInputVarNodeIds().front())
              .isIntVar());
}

void Bool2IntNode::updateState() {
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
            invariantGraphConst().varNodeConst(input()).inDomain(bool{true})
                ? Int{1}
                : Int{0});
    setState(InvariantNodeState::SUBSUMED);
  } else if (invariantGraph()
                 .varNodeConst(outputVarNodeIds().front())
                 .isFixed()) {
    invariantGraph().varNode(input()).fixToValue(
        invariantGraph()
            .varNodeConst(outputVarNodeIds().front())
            .inDomain(Int{1}));
    setState(InvariantNodeState::SUBSUMED);
  }
}

void Bool2IntNode::registerOutputVars() {
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

void Bool2IntNode::registerNode() {}

std::string Bool2IntNode::dotLangIdentifier() const { return "bool2int"; }

}  // namespace atlantis::invariantgraph
