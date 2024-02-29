#include "atlantis/invariantgraph/invariantNodes/arrayElementNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

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
      _offset(offset) {}

ArrayElementNode::ArrayElementNode(std::vector<bool>&& parVector, VarNodeId idx,
                                   VarNodeId output, Int offset)
    : InvariantNode({output}, {idx}),
      _parVector(toIntVec(std::move(parVector))),
      _offset(offset) {}

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
