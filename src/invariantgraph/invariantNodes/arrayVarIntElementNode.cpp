#include "invariantgraph/invariantNodes/arrayVarIntElementNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

ArrayVarIntElementNode::ArrayVarIntElementNode(VarNodeId b,
                                               std::vector<VarNodeId>&& as,
                                               VarNodeId output, Int offset)
    : InvariantNode({output}, {b}, std::move(as)), _offset(offset) {}

void ArrayVarIntElementNode::registerOutputVars(
    InvariantGraph& invariantGraph, propagation::SolverBase& solver) {
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()),
                _offset);
}

void ArrayVarIntElementNode::registerNode(InvariantGraph& invariantGraph,
                                          propagation::SolverBase& solver) {
  std::vector<propagation::VarId> as;
  std::transform(dynamicInputVarNodeIds().begin(),
                 dynamicInputVarNodeIds().end(), std::back_inserter(as),
                 [&](auto node) { return invariantGraph.varId(node); });

  assert(invariantGraph.varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);

  solver.makeInvariant<propagation::ElementVar>(
      solver, invariantGraph.varId(outputVarNodeIds().front()),
      invariantGraph.varId(b()), as, _offset);
}

}  // namespace atlantis::invariantgraph