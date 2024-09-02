#include "atlantis/invariantgraph/views/intScalarNode.hpp"

#include "atlantis/propagation/views/scalarView.hpp"

namespace atlantis::invariantgraph {

IntScalarNode::IntScalarNode(IInvariantGraph& graph, VarNodeId staticInput,
                             VarNodeId output, Int factor, Int offset)
    : InvariantNode(graph, {output}, {staticInput}),
      _factor(factor),
      _offset(offset) {}

void IntScalarNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(invariantGraphConst()
             .varNodeConst(outputVarNodeIds().front())
             .isIntVar());
  assert(invariantGraph()
             .varNodeConst(staticInputVarNodeIds().front())
             .isIntVar());
}

void IntScalarNode::updateState() {
  if (_factor == 0) {
    invariantGraph().varNode(outputVarNodeIds().front()).fixToValue(_offset);
    setState(InvariantNodeState::SUBSUMED);
  } else if (invariantGraph()
                 .varNodeConst(staticInputVarNodeIds().front())
                 .isFixed()) {
    invariantGraph()
        .varNode(outputVarNodeIds().front())
        .fixToValue(_factor * invariantGraph()
                                  .varNodeConst(staticInputVarNodeIds().front())
                                  .lowerBound() +
                    _offset);
    setState(InvariantNodeState::SUBSUMED);
  }
}

void IntScalarNode::registerOutputVars() {
  if (invariantGraph().varId(outputVarNodeIds().front()) ==
      propagation::NULL_ID) {
    invariantGraph()
        .varNode(outputVarNodeIds().front())
        .setVarId(solver().makeIntView<propagation::ScalarView>(
            solver(), invariantGraph().varId(input()), _factor, _offset));
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId vId) {
                       return invariantGraphConst().varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void IntScalarNode::registerNode() {}

std::ostream& IntScalarNode::dotLangEntry(std::ostream& o) const { return o; }

std::ostream& IntScalarNode::dotLangEdges(std::ostream& o) const {
  const std::string label =
      (_factor == 1 ? "" : ("* " + std::to_string(_factor))) +
      (_factor != 1 && _offset != 0 ? " " : "") +
      (_offset == 0
           ? ""
           : ((_offset < 0 ? "- " : "+ ") + std::to_string(std::abs(_offset))));

  return o << staticInputVarNodeIds().front() << " -> "
           << outputVarNodeIds().front() << "[label=\"" << label << "\"];"
           << std::endl;
}

std::string IntScalarNode::dotLangIdentifier() const { return ""; }

}  // namespace atlantis::invariantgraph
