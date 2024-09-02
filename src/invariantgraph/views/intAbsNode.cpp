#include "atlantis/invariantgraph/views/intAbsNode.hpp"

#include <map>
#include <utility>

#include "atlantis/propagation/views/intAbsView.hpp"

namespace atlantis::invariantgraph {

IntAbsNode::IntAbsNode(IInvariantGraph& graph, VarNodeId staticInput,
                       VarNodeId output)
    : InvariantNode(graph, {output}, {staticInput}) {}

void IntAbsNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(invariantGraphConst()
             .varNodeConst(outputVarNodeIds().front())
             .isIntVar());
  assert(invariantGraph()
             .varNodeConst(staticInputVarNodeIds().front())
             .isIntVar());
}

void IntAbsNode::updateState() {
  if (invariantGraph()
          .varNodeConst(staticInputVarNodeIds().front())
          .isFixed()) {
    invariantGraph()
        .varNode(outputVarNodeIds().front())
        .fixToValue(std::abs(invariantGraph()
                                 .varNodeConst(staticInputVarNodeIds().front())
                                 .lowerBound()));
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool IntAbsNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         invariantGraphConst()
                 .varNodeConst(staticInputVarNodeIds().front())
                 .lowerBound() >= 0;
}

bool IntAbsNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  invariantGraph().replaceVarNode(outputVarNodeIds().front(),
                                  staticInputVarNodeIds().front());
  return true;
}

void IntAbsNode::registerOutputVars() {
  if (invariantGraph().varId(outputVarNodeIds().front()) ==
      propagation::NULL_ID) {
    invariantGraph()
        .varNode(outputVarNodeIds().front())
        .setVarId(solver().makeIntView<propagation::IntAbsView>(
            solver(), invariantGraph().varId(input())));
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId vId) {
                       return invariantGraphConst().varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void IntAbsNode::registerNode() {}

std::string IntAbsNode::dotLangIdentifier() const { return "abs"; }

}  // namespace atlantis::invariantgraph
