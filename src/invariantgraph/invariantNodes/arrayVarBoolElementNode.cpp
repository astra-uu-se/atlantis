#include "invariantgraph/invariantNodes/arrayVarBoolElementNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

ArrayVarBoolElementNode::ArrayVarBoolElementNode(VarNodeId b,
                                                 std::vector<VarNodeId>&& as,
                                                 VarNodeId output, Int offset)
    : InvariantNode({output}, {b}, std::move(as)), _offset(offset) {}

void ArrayVarBoolElementNode::registerOutputVars(
    InvariantGraph& invariantGraph, propagation::SolverBase& solver) {
  // TODO: offset can be different than 1
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()), 1);
}

void ArrayVarBoolElementNode::registerNode(InvariantGraph& invariantGraph,
                                           propagation::SolverBase& solver) {
  std::vector<propagation::VarId> as;
  std::transform(dynamicInputVarNodeIds().begin(),
                 dynamicInputVarNodeIds().end(), std::back_inserter(as),
                 [&](auto node) { return invariantGraph.varId(node); });

  assert(invariantGraph.varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);

  solver.makeInvariant<propagation::ElementVar>(
      solver, invariantGraph.varId(outputVarNodeIds().front()),
      invariantGraph.varId(b()), as);
}

}  // namespace atlantis::invariantgraph