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

ArrayElementNode::ArrayElementNode(std::vector<Int>&& parVector, VarNodeId idx,
                                   VarNodeId output, Int offset)
    : InvariantNode({output}, {idx}),
      _parVector(std::move(parVector)),
      _offset(offset),
      _isIntVector(true) {}

ArrayElementNode::ArrayElementNode(std::vector<bool>&& parVector, VarNodeId idx,
                                   VarNodeId output, Int offset)
    : InvariantNode({output}, {idx}),
      _parVector(toIntVec(std::move(parVector))),
      _offset(offset),
      _isIntVector(false) {}

void ArrayElementNode::init(InvariantGraph& graph, const InvariantNodeId& id) {
  InvariantNode::init(graph, id);
  assert(_isIntVector ==
         graph.varNodeConst(outputVarNodeIds().front()).isIntVar());
}

void ArrayElementNode::updateState(InvariantGraph& graph) {
  const auto& idxNode = graph.varNodeConst(idx());
  if (idxNode.isFixed()) {
    auto& outputNode = graph.varNode(outputVarNodeIds().front());
    if (outputNode.isIntVar()) {
      outputNode.fixToValue(getVal(_parVector, idxNode.lowerBound(), _offset));
    } else {
      outputNode.fixToValue(getVal(_parVector, idxNode.lowerBound(), _offset) ==
                            0);
    }
    setState(InvariantNodeState::SUBSUMED);
  }
}

void ArrayElementNode::registerOutputVars(InvariantGraph& invariantGraph,
                                          propagation::SolverBase& solver) {
  if (invariantGraph.varId(outputVarNodeIds().front()) ==
      propagation::NULL_ID) {
    assert(invariantGraph.varId(idx()) != propagation::NULL_ID);
    invariantGraph.varNode(outputVarNodeIds().front())
        .setVarId(solver.makeIntView<propagation::ElementConst>(
            solver, invariantGraph.varId(idx()), std::vector<Int>(_parVector),
            _offset));
  }
}

void ArrayElementNode::registerNode(InvariantGraph&, propagation::SolverBase&) {
}

}  // namespace atlantis::invariantgraph
