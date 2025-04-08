#include "atlantis/invariantgraph/views/intModViewNode.hpp"

#include <algorithm>

#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/modView.hpp"

namespace atlantis::invariantgraph {

IntModViewNode::IntModViewNode(InvariantGraph& graph, VarNodeId staticInput,
                               VarNodeId output, Int denominator)
    : InvariantNode(graph, {output}, {staticInput}),
      _denominator(std::abs(denominator)) {}

void IntModViewNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(invariantGraphConst()
             .varNodeConst(outputVarNodeIds().front())
             .isIntVar());
  assert(invariantGraph()
             .varNodeConst(staticInputVarNodeIds().front())
             .isIntVar());
}

void IntModViewNode::updateState() {
  if (invariantGraph()
          .varNodeConst(staticInputVarNodeIds().front())
          .isFixed()) {
    invariantGraph()
        .varNode(outputVarNodeIds().front())
        .fixToValue(invariantGraph()
                        .varNodeConst(staticInputVarNodeIds().front())
                        .lowerBound() %
                    _denominator);
    setState(InvariantNodeState::SUBSUMED);
  }
}

void IntModViewNode::registerOutputVars() {
  if (invariantGraph().varId(outputVarNodeIds().front()) ==
      propagation::NULL_ID) {
    invariantGraph()
        .varNode(outputVarNodeIds().front())
        .setVarId(solver().makeIntView<propagation::ModView>(
            solver(), invariantGraph().varId(input()), _denominator));
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).varId() !=
               propagation::NULL_ID;
      }));
}

void IntModViewNode::registerNode() {}

std::string IntModViewNode::dotLangIdentifier() const {
  return "% " + std::to_string(_denominator);
}

}  // namespace atlantis::invariantgraph
