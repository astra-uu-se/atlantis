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
      _cover(std::move(cover)),
      _countOffsets(_cover.size(), 0) {}

void GlobalCardinalityNode::registerOutputVars(
    InvariantGraph& invariantGraph, propagation::SolverBase& solver) {
  for (const VarNodeId& countOutput : outputVarNodeIds()) {
    if (invariantGraph.varId(countOutput) == propagation::NULL_ID) {
      makeSolverVar(solver, invariantGraph.varNode(countOutput));
    }
  }
}

void GlobalCardinalityNode::updateState(InvariantGraph& graph) {
  std::vector<bool> coverIsFixed(_cover.size(), true);
  std::vector<VarNodeId> varsToRemove;
  varsToRemove.reserve(staticInputVarNodeIds().size());
  for (const auto& input : staticInputVarNodeIds()) {
    const auto& var = graph.varNodeConst(input);
    if (var.isFixed()) {
      for (size_t i = 0; i < _cover.size(); ++i) {
        if (var.lowerBound() == _cover[i]) {
          ++_countOffsets[i];
        }
      }
      varsToRemove.emplace_back(input);
    } else {
      for (size_t i = 0; i < _cover.size(); ++i) {
        coverIsFixed[i] = coverIsFixed[i] && !var.inDomain(_cover[i]);
      }
    }
  }
  for (const auto& input : varsToRemove) {
    removeStaticInputVarNode(graph.varNode(input));
  }
  for (Int i = static_cast<Int>(_cover.size()) - 1; i >= 0; --i) {
    if (coverIsFixed[i]) {
      graph.varNode(outputVarNodeIds()[i]).fixToValue(_countOffsets[i]);
      _countOffsets.erase(_countOffsets.begin() + i);
      _cover.erase(_cover.begin() + i);
    }
  }
  if (_cover.empty()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool GlobalCardinalityNode::canBeReplaced(const InvariantGraph&) const {
  return _cover.size() <= 1;
}

bool GlobalCardinalityNode::replace(InvariantGraph& invariantGraph) {
  if (!canBeReplaced(invariantGraph)) {
    return false;
  }
  assert(_cover.size() == 1);
  invariantGraph.addInvariantNode(std::make_unique<IntCountNode>(
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
