#include "atlantis/invariantgraph/invariantNodes/boolLinearNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/boolLinear.hpp"
#include "atlantis/propagation/views/intOffsetView.hpp"
#include "atlantis/propagation/views/scalarView.hpp"

namespace atlantis::invariantgraph {

BoolLinearNode::BoolLinearNode(std::vector<Int>&& coeffs,
                               std::vector<VarNodeId>&& vars, VarNodeId output)
    : InvariantNode({output}, std::move(vars)), _coeffs(std::move(coeffs)) {}

void BoolLinearNode::registerOutputVars(InvariantGraph& invariantGraph,
                                        propagation::SolverBase& solver) {
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()));
}

void BoolLinearNode::propagate(InvariantGraph& graph) {
  std::vector<bool> removedVars(staticInputVarNodeIds().size(), false);
  Int minSum = 0;
  Int maxSum = 0;
  for (size_t i = 0; i < _coeffs.size(); ++i) {
    assert(_coeffs[i] != 0);
    if (graph.isFixed(staticInputVarNodeIds().at(i))) {
      removedVars[i] = true;
      if (graph.inDomain(staticInputVarNodeIds().at(i), false)) {
        continue;
      }
    }
    if (_coeffs[i] > 0) {
      maxSum += _coeffs[i];
    } else {
      minSum += _coeffs[i];
    }
  }
  if (minSum > maxSum) {
    setState(InvariantNodeState::INFEASIBLE);
    return;
  }
  for (Int i = static_cast<Int>(_coeffs.size()) - 1; i >= 0; --i) {
    if (removedVars[i]) {
      removeStaticInputVarNode(graph.varNode(staticInputVarNodeIds().at(i)));
      _coeffs.erase(_coeffs.begin() + i);
    }
  }
  graph.removeValuesBelow(outputVarNodeIds().front(), minSum);
  graph.removeValuesAbove(outputVarNodeIds().front(), maxSum);
  if (minSum == maxSum) {
    setState(InvariantNodeState::SUBSUMED);
  }
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
      solver, invariantGraph.varId(outputVarNodeIds().front()),
      std::vector<Int>(_coeffs), std::move(solverVars));
}

const std::vector<Int>& BoolLinearNode::coeffs() const { return _coeffs; }

}  // namespace atlantis::invariantgraph
