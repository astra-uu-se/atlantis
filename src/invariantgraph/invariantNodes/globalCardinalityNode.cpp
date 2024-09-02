#include "atlantis/invariantgraph/invariantNodes/globalCardinalityNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantNodes/intCountNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intAllEqualNode.hpp"
#include "atlantis/propagation/invariants/globalCardinalityOpen.hpp"
#include "atlantis/propagation/views/equalConst.hpp"
#include "atlantis/propagation/views/intOffsetView.hpp"

namespace atlantis::invariantgraph {

GlobalCardinalityNode::GlobalCardinalityNode(std::vector<VarNodeId>&& inputs,
                                             std::vector<Int>&& cover,
                                             std::vector<VarNodeId>&& counts)
    : InvariantNode(std::move(counts), std::move(inputs)),
      _cover(std::move(cover)),
      _countOffsets(_cover.size(), 0),
      _intermediate(_cover.size(), propagation::NULL_ID) {}

void GlobalCardinalityNode::init(InvariantGraph& graph,
                                 const InvariantNodeId& id) {
  InvariantNode::init(graph, id);
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).isIntVar();
                     }));
  assert(std::all_of(staticInputVarNodeIds().begin(),
                     staticInputVarNodeIds().end(), [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).isIntVar();
                     }));
}

void GlobalCardinalityNode::updateState(InvariantGraph& graph) {
  // GCC can define the same output multiple times. Therefore, split all outputs
  // that are defined multiple times:
  std::vector<std::pair<VarNodeId, VarNodeId>> replacedOutputs =
      splitOutputVarNodes(graph);
  for (const auto& [oldVarNodeId, newVarNodeId] : replacedOutputs) {
    assert(graph.varNodeConst(oldVarNodeId).definingNodes().contains(id()));
    assert(graph.varNodeConst(newVarNodeId).definingNodes().contains(id()));
    graph.addInvariantNode(std::make_unique<IntAllEqualNode>(
        oldVarNodeId, newVarNodeId, true, true));
  }

  std::vector<bool> coverIsFixed(_cover.size(), true);
  std::vector<VarNodeId> inputsToRemove;
  inputsToRemove.reserve(staticInputVarNodeIds().size());
  for (const auto& input : staticInputVarNodeIds()) {
    const auto& var = graph.varNodeConst(input);
    if (var.isFixed()) {
      for (size_t i = 0; i < _cover.size(); ++i) {
        if (var.lowerBound() == _cover[i]) {
          ++_countOffsets[i];
        }
      }
      inputsToRemove.emplace_back(input);
    } else {
      for (size_t i = 0; i < _cover.size(); ++i) {
        coverIsFixed[i] = coverIsFixed[i] && !var.inDomain(_cover[i]);
      }
    }
  }
  for (const auto& input : inputsToRemove) {
    removeStaticInputVarNode(graph.varNode(input));
  }
  std::vector<VarNodeId> outputsToRemove;
  outputsToRemove.reserve(outputVarNodeIds().size());
  for (Int i = static_cast<Int>(_cover.size()) - 1; i >= 0; --i) {
    if (coverIsFixed[i]) {
      graph.varNode(outputVarNodeIds()[i]).fixToValue(_countOffsets[i]);
      _countOffsets.erase(_countOffsets.begin() + i);
      _cover.erase(_cover.begin() + i);
      _intermediate.erase(_intermediate.begin() + i);
      outputsToRemove.emplace_back(outputVarNodeIds()[i]);
    }
  }
  for (const auto& output : outputsToRemove) {
    removeOutputVarNode(graph.varNode(output));
  }
  if (_cover.empty()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool GlobalCardinalityNode::canBeReplaced(const InvariantGraph&) const {
  return state() == InvariantNodeState::ACTIVE && _cover.size() <= 1;
}

bool GlobalCardinalityNode::replace(InvariantGraph& graph) {
  if (!canBeReplaced(graph)) {
    return false;
  }
  assert(_cover.size() == 1);
  graph.addInvariantNode(std::make_unique<IntCountNode>(
      std::vector<VarNodeId>(staticInputVarNodeIds()), _cover.front(),
      outputVarNodeIds().front()));
  return true;
}

void GlobalCardinalityNode::registerOutputVars(
    InvariantGraph& graph, propagation::SolverBase& solver) {
  for (size_t i = 0; i < _cover.size(); ++i) {
    const bool isDuplicate = std::any_of(
        outputVarNodeIds().begin(), outputVarNodeIds().begin() + i,
        [&](const VarNodeId& vId) { return vId == outputVarNodeIds().at(i); });

    assert(!isDuplicate ||
           graph.varNodeConst(outputVarNodeIds().at(i)).isFixed());

    if (isDuplicate) {
      assert(graph.varNodeConst(outputVarNodeIds().at(i)).varId() !=
             propagation::NULL_ID);
      _intermediate.at(i) = solver.makeIntVar(0, 0, 0);
      solver.makeIntView<propagation::EqualConst>(
          solver, _intermediate.at(i),
          graph.varNodeConst(outputVarNodeIds().at(i)).lowerBound() -
              _countOffsets[i]);
    } else if (_countOffsets[i] == 0) {
      assert(graph.varNodeConst(outputVarNodeIds().at(i)).varId() ==
             propagation::NULL_ID);
      makeSolverVar(solver, graph.varNode(outputVarNodeIds().at(i)));
    } else {
      assert(graph.varNodeConst(outputVarNodeIds().at(i)).varId() ==
             propagation::NULL_ID);
      _intermediate.at(i) = solver.makeIntVar(0, 0, 0);
      graph.varNode(outputVarNodeIds().at(i))
          .setVarId(solver.makeIntView<propagation::IntOffsetView>(
              solver, _intermediate.at(i), _countOffsets[i]));
    }
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void GlobalCardinalityNode::registerNode(InvariantGraph& graph,
                                         propagation::SolverBase& solver) {
  std::vector<propagation::VarId> inputVarIds;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(inputVarIds),
                 [&](const auto& id) { return graph.varId(id); });

  std::vector<propagation::VarId> outputVarIds;
  outputVarIds.reserve(outputVarNodeIds().size());
  for (size_t i = 0; i < _cover.size(); ++i) {
    outputVarIds.emplace_back(_intermediate.at(i) == propagation::NULL_ID
                                  ? graph.varId(outputVarNodeIds().at(i))
                                  : _intermediate.at(i));
  }

  solver.makeInvariant<propagation::GlobalCardinalityOpen>(
      solver, std::move(outputVarIds), std::move(inputVarIds),
      std::vector<Int>(_cover));
}

std::string GlobalCardinalityNode::dotLangIdentifier() const {
  std::string s{"global_cardinality ["};
  for (size_t i = 0; i < _cover.size(); ++i) {
    s += _cover.at(i);
    if (i < _cover.size() - 1) {
      s += ", ";
    }
  }
  s += ']';
  return s;
}

}  // namespace atlantis::invariantgraph
