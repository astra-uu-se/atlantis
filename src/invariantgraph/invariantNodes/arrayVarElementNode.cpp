#include "atlantis/invariantgraph/invariantNodes/arrayVarElementNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

ArrayVarElementNode::ArrayVarElementNode(VarNodeId idx,
                                         std::vector<VarNodeId>&& varVector,
                                         VarNodeId output, Int offset)
    : InvariantNode({output}, {idx}, std::move(varVector)), _offset(offset) {}

void ArrayVarElementNode::registerOutputVars(InvariantGraph& invariantGraph,
                                             propagation::SolverBase& solver) {
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()),
                _offset);
}

void ArrayVarElementNode::registerNode(InvariantGraph& invariantGraph,
                                       propagation::SolverBase& solver) {
  std::vector<propagation::VarId> varVector;
  std::transform(dynamicInputVarNodeIds().begin(),
                 dynamicInputVarNodeIds().end(), std::back_inserter(varVector),
                 [&](auto node) { return invariantGraph.varId(node); });

  assert(invariantGraph.varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);

  solver.makeInvariant<propagation::ElementVar>(
      solver, invariantGraph.varId(outputVarNodeIds().front()),
      invariantGraph.varId(idx()), std::move(varVector), _offset);
}

}  // namespace atlantis::invariantgraph
