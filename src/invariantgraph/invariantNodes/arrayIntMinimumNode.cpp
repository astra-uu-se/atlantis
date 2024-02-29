#include "atlantis/invariantgraph/invariantNodes/arrayIntMinimumNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

ArrayIntMinimumNode::ArrayIntMinimumNode(std::vector<VarNodeId>&& vars,
                                         VarNodeId output)
    : InvariantNode({output}, std::move(vars)) {}

void ArrayIntMinimumNode::registerOutputVars(InvariantGraph& invariantGraph,
                                             propagation::SolverBase& solver) {
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()));
}

void ArrayIntMinimumNode::registerNode(InvariantGraph& invariantGraph,
                                       propagation::SolverBase& solver) {
  std::vector<propagation::VarId> solverVars;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars),
                 [&](const auto& node) { return invariantGraph.varId(node); });

  assert(invariantGraph.varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);
  solver.makeInvariant<propagation::MinSparse>(
      solver, invariantGraph.varId(outputVarNodeIds().front()),
      std::move(solverVars));
}

}  // namespace atlantis::invariantgraph
