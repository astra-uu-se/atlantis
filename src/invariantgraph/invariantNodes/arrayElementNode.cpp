#include "atlantis/invariantgraph/invariantNodes/arrayElementNode.hpp"

#include "../parseHelper.hpp"
#include "atlantis/propagation/views/elementConst.hpp"

namespace atlantis::invariantgraph {

static Int getVal(const std::vector<Int>& parVector, Int idx, Int offset) {
  assert(0 <= idx - offset &&
         idx - offset < static_cast<Int>(parVector.size()));
  return parVector.at(idx - offset);
}

static std::vector<Int> toIntVec(std::vector<bool>&& boolVec) {
  std::vector<Int> intVec;
  intVec.reserve(boolVec.size());
  for (bool par : boolVec) {
    intVec.emplace_back(par ? 0 : 1);
  }
  return intVec;
}

ArrayElementNode::ArrayElementNode(IInvariantGraph& graph,
                                   std::vector<Int>&& parVector, VarNodeId idx,
                                   VarNodeId output, Int offset,
                                   bool isIntVector)
    : InvariantNode(graph, {output}, {idx}),
      _parVector(std::move(parVector)),
      _offset(offset),
      _isIntVector(isIntVector) {}

ArrayElementNode::ArrayElementNode(IInvariantGraph& graph,
                                   std::vector<bool>&& parVector, VarNodeId idx,
                                   VarNodeId output, Int offset)
    : InvariantNode(graph, {output}, {idx}),
      _parVector(toIntVec(std::move(parVector))),
      _offset(offset),
      _isIntVector(false) {}

void ArrayElementNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(_isIntVector == invariantGraphConst()
                             .varNodeConst(outputVarNodeIds().front())
                             .isIntVar());
}

void ArrayElementNode::updateState() {
  const auto& idxNode = invariantGraphConst().varNodeConst(idx());
  if (idxNode.isFixed()) {
    auto& outputNode = invariantGraph().varNode(outputVarNodeIds().front());
    if (outputNode.isIntVar()) {
      outputNode.fixToValue(getVal(_parVector, idxNode.lowerBound(), _offset));
    } else {
      outputNode.fixToValue(getVal(_parVector, idxNode.lowerBound(), _offset) ==
                            0);
    }
    setState(InvariantNodeState::SUBSUMED);
  }
}

void ArrayElementNode::registerOutputVars() {
  if (invariantGraph().varId(outputVarNodeIds().front()) ==
      propagation::NULL_ID) {
    assert(invariantGraph().varId(idx()) != propagation::NULL_ID);
    invariantGraph()
        .varNode(outputVarNodeIds().front())
        .setVarId(solver().makeIntView<propagation::ElementConst>(
            solver(), invariantGraph().varId(idx()),
            std::vector<Int>(_parVector), _offset));
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId vId) {
                       return invariantGraphConst().varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void ArrayElementNode::registerNode() {}

std::string ArrayElementNode::dotLangIdentifier() const { return "element"; }

}  // namespace atlantis::invariantgraph
