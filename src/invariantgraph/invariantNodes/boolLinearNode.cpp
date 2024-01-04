#include "invariantgraph/invariantNodes/boolLinearNode.hpp"

#include "../parseHelper.hpp"

namespace atlantis::invariantgraph {

BoolLinearNode::BoolLinearNode(std::vector<Int>&& coeffs,
                               std::vector<VarNodeId>&& vars, VarNodeId output)
    : InvariantNode({output}, std::move(vars)), _coeffs(std::move(coeffs)) {}

void BoolLinearNode::registerOutputVars(InvariantGraph& invariantGraph,
                                        propagation::SolverBase& solver) {
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()));
}

void BoolLinearNode::registerNode(InvariantGraph& invariantGraph,
                                  propagation::SolverBase& solver) {
  assert(invariantGraph.varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);

  std::vector<propagation::VarId> solverVars;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars),
                 [&](const VarNodeId varNodeId) {
                   return invariantGraph.varId(varNodeId);
                 });
  solver.makeInvariant<propagation::BoolLinear>(
      solver, invariantGraph.varId(outputVarNodeIds().front()), _coeffs,
      solverVars);
}

}  // namespace atlantis::invariantgraph