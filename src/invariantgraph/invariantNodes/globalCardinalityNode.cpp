#include "atlantis/invariantgraph/invariantNodes/globalCardinalityNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantNodes/intCountNode.hpp"
#include "atlantis/propagation/invariants/exists.hpp"
#include "atlantis/propagation/invariants/globalCardinalityOpen.hpp"
#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/views/equalConst.hpp"
#include "atlantis/propagation/views/notEqualConst.hpp"
#include "atlantis/propagation/violationInvariants/equal.hpp"
#include "atlantis/propagation/violationInvariants/notEqual.hpp"

namespace atlantis::invariantgraph {

GlobalCardinalityNode::GlobalCardinalityNode(std::vector<VarNodeId>&& inputs,
                                             std::vector<Int>&& cover,
                                             std::vector<VarNodeId>&& counts)
    : InvariantNode(std::move(counts), std::move(inputs)),
      _cover(std::move(cover)) {}

void GlobalCardinalityNode::registerOutputVars(
    InvariantGraph& invariantGraph, propagation::SolverBase& solver) {
  for (const VarNodeId& countOutput : outputVarNodeIds()) {
    if (invariantGraph.varId(countOutput) == propagation::NULL_ID) {
      makeSolverVar(solver, invariantGraph.varNode(countOutput));
    }
  }
}

void GlobalCardinalityNode::propagate(InvariantGraph& graph) {
  if (_cover.size() == 1) {
    setState(InvariantNodeState::REPLACABLE);
    return;
  }
  std::vector<VarNodeId> removedVars;
  removedVars.reserve(staticInputVarNodeIds().size());

  std::vector<size_t> minCounts(_cover.size(), 0);
  std::vector<size_t> maxCounts(_cover.size(), 0);
  for (const VarNodeId& input : staticInputVarNodeIds()) {
    bool relevant = false;
    for (size_t valIndex = 0; valIndex < _cover.size(); ++valIndex) {
      if (!graph.inDomain(input, _cover[valIndex])) {
        continue;
      }
      relevant = true;
      if (graph.isFixed(input)) {
        ++minCounts[valIndex];
      }
      ++maxCounts[valIndex];
    }
    if (!relevant) {
      removedVars.emplace_back(input);
    }
  }

  bool inputsFixed = true;

  for (size_t valIndex = 0; valIndex < _cover.size(); ++valIndex) {
    graph.removeValuesBelow(outputVarNodeIds().at(valIndex),
                            minCounts[valIndex]);
    graph.removeValuesAbove(outputVarNodeIds().at(valIndex),
                            maxCounts[valIndex]);
    inputsFixed = inputsFixed && minCounts[valIndex] == maxCounts[valIndex];
  }
  if (inputsFixed) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool GlobalCardinalityNode::replace(InvariantGraph& invariantGraph) {
  if (state() != InvariantNodeState::REPLACABLE) {
    return false;
  }
  assert(_cover.size() == 1);
  invariantGraph.replaceInvariantNode(
      id(), std::make_unique<IntCountNode>(
                std::vector<VarNodeId>(staticInputVarNodeIds()), _cover.front(),
                outputVarNodeIds().front()));
  return true;
}

void GlobalCardinalityNode::registerNode(InvariantGraph& invariantGraph,
                                         propagation::SolverBase& solver) {
  std::vector<propagation::VarId> inputVarIds;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(inputVarIds),
                 [&](const auto& id) { return invariantGraph.varId(id); });

  std::vector<propagation::VarId> outputVarIds;
  std::transform(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                 std::back_inserter(outputVarIds),
                 [&](const auto& id) { return invariantGraph.varId(id); });

  solver.makeInvariant<propagation::GlobalCardinalityOpen>(
      solver, std::move(outputVarIds), std::move(inputVarIds),
      std::vector<Int>(_cover));
}

}  // namespace atlantis::invariantgraph
