#include "atlantis/invariantgraph/invariantNodes/arrayIntMinimumNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/minSparse.hpp"

namespace atlantis::invariantgraph {

ArrayIntMinimumNode::ArrayIntMinimumNode(VarNodeId a, VarNodeId b,
                                         VarNodeId output)
    : ArrayIntMinimumNode(std::vector<VarNodeId>{a, b}, output) {}

ArrayIntMinimumNode::ArrayIntMinimumNode(std::vector<VarNodeId>&& vars,
                                         VarNodeId output)
    : InvariantNode({output}, std::move(vars)),
      _ub(std::numeric_limits<Int>::max()) {}

void ArrayIntMinimumNode::registerOutputVars(InvariantGraph& invariantGraph,
                                             propagation::SolverBase& solver) {
  makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()));
}

void ArrayIntMinimumNode::updateState(InvariantGraph& graph) {
  Int lb = _ub;
  for (const auto& input : staticInputVarNodeIds()) {
    lb = std::min(lb, graph.varNodeConst(input).lowerBound());
    _ub = std::min(_ub, graph.varNodeConst(input).upperBound());
  }
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());

  for (const auto& input : staticInputVarNodeIds()) {
    if (graph.varNodeConst(input).lowerBound() >= _ub) {
      varsToRemove.emplace_back(input);
    }
  }
  for (const auto& input : varsToRemove) {
    removeStaticInputVarNode(graph.varNode(input));
  }
  auto& outputVar = graph.varNode(outputVarNodeIds().front());
  outputVar.removeValuesBelow(lb);
  outputVar.removeValuesAbove(_ub);
  if (staticInputVarNodeIds().size() == 0 || outputVar.isFixed()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool ArrayIntMinimumNode::canBeReplaced(const InvariantGraph&) const {
  return staticInputVarNodeIds().size() <= 1;
}

bool ArrayIntMinimumNode::replace(InvariantGraph& invariantGraph) {
  if (!canBeReplaced(invariantGraph)) {
    return false;
  }
  if (staticInputVarNodeIds().size() == 1) {
    invariantGraph.replaceVarNode(outputVarNodeIds().front(),
                                  staticInputVarNodeIds().front());
  }
  return true;
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
