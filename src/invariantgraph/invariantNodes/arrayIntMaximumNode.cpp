#include "atlantis/invariantgraph/invariantNodes/arrayIntMaximumNode.hpp"

#include <algorithm>
#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/propagation/invariants/maxSparse.hpp"
#include "atlantis/propagation/views/intMaxView.hpp"

namespace atlantis::invariantgraph {

ArrayIntMaximumNode::ArrayIntMaximumNode(VarNodeId a, VarNodeId b,
                                         VarNodeId output)
    : ArrayIntMaximumNode(std::vector<VarNodeId>{a, b}, output) {}

ArrayIntMaximumNode::ArrayIntMaximumNode(std::vector<VarNodeId>&& vars,
                                         VarNodeId output)
    : InvariantNode({output}, std::move(vars)),
      _lb{std::numeric_limits<Int>::min()} {}

void ArrayIntMaximumNode::updateState(InvariantGraph& graph) {
  Int ub = _lb;
  for (const auto& input : staticInputVarNodeIds()) {
    _lb = std::max(_lb, graph.varNodeConst(input).lowerBound());
    ub = std::max(ub, graph.varNodeConst(input).upperBound());
  }
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());

  for (const auto& input : staticInputVarNodeIds()) {
    if (graph.varNodeConst(input).upperBound() <= _lb) {
      varsToRemove.emplace_back(input);
    }
  }
  for (const auto& input : varsToRemove) {
    removeStaticInputVarNode(graph.varNode(input));
  }
  auto& outputVar = graph.varNode(outputVarNodeIds().front());
  outputVar.removeValuesBelow(_lb);
  outputVar.removeValuesAbove(ub);
  if (staticInputVarNodeIds().size() == 0 || outputVar.isFixed()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool ArrayIntMaximumNode::canBeReplaced(const InvariantGraph& graph) const {
  if (state() != InvariantNodeState::ACTIVE ||
      staticInputVarNodeIds().size() > 1) {
    return false;
  }
  if (staticInputVarNodeIds().empty()) {
    return true;
  }
  return _lb <=
         graph.varNodeConst(staticInputVarNodeIds().front()).lowerBound();
}

bool ArrayIntMaximumNode::replace(InvariantGraph& invariantGraph) {
  if (!canBeReplaced(invariantGraph)) {
    return false;
  }
  if (staticInputVarNodeIds().size() == 1) {
    invariantGraph.replaceVarNode(outputVarNodeIds().front(),
                                  staticInputVarNodeIds().front());
  }
  return true;
}

void ArrayIntMaximumNode::registerOutputVars(InvariantGraph& invariantGraph,
                                             propagation::SolverBase& solver) {
  if (staticInputVarNodeIds().empty()) {
    return;
  } else if (staticInputVarNodeIds().size() == 1) {
    invariantGraph.varNode(outputVarNodeIds().front())
        .setVarId(solver.makeIntView<propagation::IntMaxView>(
            solver, invariantGraph.varId(staticInputVarNodeIds().front()),
            _lb));
  } else {
    makeSolverVar(solver, invariantGraph.varNode(outputVarNodeIds().front()));
  }
}

void ArrayIntMaximumNode::registerNode(InvariantGraph& invariantGraph,
                                       propagation::SolverBase& solver) {
  if (staticInputVarNodeIds().size() <= 1) {
    return;
  }
  std::vector<propagation::VarId> solverVars;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(solverVars),
                 [&](const auto& node) { return invariantGraph.varId(node); });

  assert(invariantGraph.varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);
  solver.makeInvariant<propagation::MaxSparse>(
      solver, invariantGraph.varId(outputVarNodeIds().front()),
      std::move(solverVars));
}

}  // namespace atlantis::invariantgraph
