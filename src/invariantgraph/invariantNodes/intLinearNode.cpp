#include "atlantis/invariantgraph/invariantNodes/intLinearNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/views/intScalarNode.hpp"
#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/views/intOffsetView.hpp"

namespace atlantis::invariantgraph {

IntLinearNode::IntLinearNode(std::vector<Int>&& coeffs,
                             std::vector<VarNodeId>&& vars, VarNodeId output)
    : InvariantNode({output}, std::move(vars)), _coeffs(std::move(coeffs)) {}

void IntLinearNode::registerOutputVars(InvariantGraph& invariantGraph,
                                       propagation::SolverBase& solver) {
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()));
}

void IntLinearNode::propagate(InvariantGraph& graph) {
  std::vector<bool> removedVars(staticInputVarNodeIds().size(), false);
  Int newOffset = _offset;

  Int minSum = _offset;
  Int maxSum = _offset;

  for (size_t i = 0; i < _coeffs.size(); ++i) {
    assert(_coeffs[i] != 0);
    Int v1 = graph.lowerBound(staticInputVarNodeIds().at(i)) * _coeffs[i];
    Int v2 = graph.upperBound(staticInputVarNodeIds().at(i)) * _coeffs[i];
    minSum += std::min(v1, v2);
    maxSum += std::max(v1, v2);
    if (graph.isFixed(staticInputVarNodeIds().at(i))) {
      removedVars[i] = true;
      newOffset += graph.lowerBound(staticInputVarNodeIds().at(i)) * _coeffs[i];
    }
  }
  _offset = newOffset;

  for (Int i = static_cast<Int>(_coeffs.size()) - 1; i >= 0; --i) {
    if (removedVars[i]) {
      removeStaticInputVarNode(graph.varNode(staticInputVarNodeIds().at(i)));
      _coeffs.erase(_coeffs.begin() + i);
    }
  }

  graph.removeValuesBelow(outputVarNodeIds().front(), minSum);
  graph.removeValuesAbove(outputVarNodeIds().front(), maxSum);
  if (minSum == maxSum) {
    assert(staticInputVarNodeIds().empty());
    setState(InvariantNodeState::SUBSUMED);
  } else if (staticInputVarNodeIds().size() == 1) {
    setState(InvariantNodeState::REPLACABLE);
  }
}

bool IntLinearNode::replace(InvariantGraph& invariantGraph) {
  if (state() != InvariantNodeState::REPLACABLE) {
    return false;
  }
  assert(staticInputVarNodeIds().size() == 1);
  if (_coeffs.front() == 1 && _offset == 0) {
    invariantGraph.replaceVarNode(outputVarNodeIds().front(),
                                  staticInputVarNodeIds().front());
    deactivate(invariantGraph);
  } else {
    invariantGraph.replaceInvariantNode(
        id(), std::make_unique<IntScalarNode>(staticInputVarNodeIds().front(),
                                              outputVarNodeIds().front(),
                                              _coeffs.front(), _offset));
  }
  return true;
}

void IntLinearNode::registerNode(InvariantGraph& invariantGraph,
                                 propagation::SolverBase& solver) {
  assert(invariantGraph.varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);

  std::vector<propagation::VarId> solverVars;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars),
                 [&](const VarNodeId varNodeId) {
                   return invariantGraph.varId(varNodeId);
                 });
  solver.makeInvariant<propagation::Linear>(
      solver, invariantGraph.varId(outputVarNodeIds().front()),
      std::vector<Int>(_coeffs), std::move(solverVars));
}

const std::vector<Int>& IntLinearNode::coeffs() const { return _coeffs; }

}  // namespace atlantis::invariantgraph
