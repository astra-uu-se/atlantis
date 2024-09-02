#include "atlantis/invariantgraph/invariantNodes/globalCardinalityNode.hpp"

#include <utility>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantNodes/intCountNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intAllEqualNode.hpp"
#include "atlantis/propagation/invariants/globalCardinalityOpen.hpp"
#include "atlantis/propagation/views/equalConst.hpp"
#include "atlantis/propagation/views/intOffsetView.hpp"

namespace atlantis::invariantgraph {

GlobalCardinalityNode::GlobalCardinalityNode(IInvariantGraph& graph,
                                             std::vector<VarNodeId>&& inputs,
                                             std::vector<Int>&& cover,
                                             std::vector<VarNodeId>&& counts)
    : InvariantNode(graph, std::move(counts), std::move(inputs)),
      _cover(std::move(cover)),
      _countOffsets(_cover.size(), 0),
      _intermediate(_cover.size(), propagation::NULL_ID) {}

void GlobalCardinalityNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(
      std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                  [&](const VarNodeId vId) {
                    return invariantGraphConst().varNodeConst(vId).isIntVar();
                  }));
  assert(
      std::all_of(staticInputVarNodeIds().begin(),
                  staticInputVarNodeIds().end(), [&](const VarNodeId vId) {
                    return invariantGraphConst().varNodeConst(vId).isIntVar();
                  }));
}

void GlobalCardinalityNode::updateState() {
  // GCC can define the same output multiple times. Therefore, split all outputs
  // that are defined multiple times:
  std::vector<std::pair<VarNodeId, VarNodeId>> replacedOutputs =
      splitOutputVarNodes();
  for (const auto& [oldVarNodeId, newVarNodeId] : replacedOutputs) {
    assert(invariantGraph()
               .varNodeConst(oldVarNodeId)
               .definingNodes()
               .contains(id()));
    assert(invariantGraph()
               .varNodeConst(newVarNodeId)
               .definingNodes()
               .contains(id()));
    invariantGraph().addInvariantNode(std::make_shared<IntAllEqualNode>(
        invariantGraph(), oldVarNodeId, newVarNodeId, true, true));
  }

  std::vector<bool> coverIsFixed(_cover.size(), true);
  std::vector<VarNodeId> inputsToRemove;
  inputsToRemove.reserve(staticInputVarNodeIds().size());
  for (const auto& input : staticInputVarNodeIds()) {
    const auto& var = invariantGraphConst().varNodeConst(input);
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
    removeStaticInputVarNode(input);
  }
  std::vector<VarNodeId> outputsToRemove;
  outputsToRemove.reserve(outputVarNodeIds().size());
  for (Int i = static_cast<Int>(_cover.size()) - 1; i >= 0; --i) {
    if (coverIsFixed[i]) {
      invariantGraph()
          .varNode(outputVarNodeIds()[i])
          .fixToValue(_countOffsets[i]);
      _countOffsets.erase(_countOffsets.begin() + i);
      _cover.erase(_cover.begin() + i);
      _intermediate.erase(_intermediate.begin() + i);
      outputsToRemove.emplace_back(outputVarNodeIds()[i]);
    }
  }
  for (const auto& output : outputsToRemove) {
    removeOutputVarNode(output);
  }
  if (_cover.empty()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

bool GlobalCardinalityNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE && _cover.size() <= 1;
}

bool GlobalCardinalityNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  assert(_cover.size() == 1);
  invariantGraph().addInvariantNode(std::make_shared<IntCountNode>(
      invariantGraph(), std::vector<VarNodeId>(staticInputVarNodeIds()),
      _cover.front(), outputVarNodeIds().front()));
  return true;
}

void GlobalCardinalityNode::registerOutputVars() {
  for (size_t i = 0; i < _cover.size(); ++i) {
    const bool isDuplicate = std::any_of(
        outputVarNodeIds().begin(), outputVarNodeIds().begin() + i,
        [&](const VarNodeId vId) { return vId == outputVarNodeIds().at(i); });

    assert(
        !isDuplicate ||
        invariantGraphConst().varNodeConst(outputVarNodeIds().at(i)).isFixed());

    if (isDuplicate) {
      assert(invariantGraphConst()
                 .varNodeConst(outputVarNodeIds().at(i))
                 .varId() != propagation::NULL_ID);
      _intermediate.at(i) = solver().makeIntVar(0, 0, 0);
      solver().makeIntView<propagation::EqualConst>(
          solver(), _intermediate.at(i),
          invariantGraphConst()
                  .varNodeConst(outputVarNodeIds().at(i))
                  .lowerBound() -
              _countOffsets[i]);
    } else if (_countOffsets[i] == 0) {
      assert(invariantGraphConst()
                 .varNodeConst(outputVarNodeIds().at(i))
                 .varId() == propagation::NULL_ID);
      makeSolverVar(outputVarNodeIds().at(i));
    } else {
      assert(invariantGraphConst()
                 .varNodeConst(outputVarNodeIds().at(i))
                 .varId() == propagation::NULL_ID);
      _intermediate.at(i) = solver().makeIntVar(0, 0, 0);
      invariantGraph()
          .varNode(outputVarNodeIds().at(i))
          .setVarId(solver().makeIntView<propagation::IntOffsetView>(
              solver(), _intermediate.at(i), _countOffsets[i]));
    }
  }
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId vId) {
                       return invariantGraphConst().varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

void GlobalCardinalityNode::registerNode() {
  std::vector<propagation::VarViewId> inputVarIds;
  std::transform(staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
                 std::back_inserter(inputVarIds),
                 [&](const auto& id) { return invariantGraph().varId(id); });

  std::vector<propagation::VarViewId> outputVarIds;
  outputVarIds.reserve(outputVarNodeIds().size());
  for (size_t i = 0; i < _cover.size(); ++i) {
    assert(_intermediate.at(i) == propagation::NULL_ID ||
           _intermediate.at(i).isVar());
    assert(_intermediate.at(i) == propagation::NULL_ID
               ? invariantGraph().varId(outputVarNodeIds().at(i)).isVar()
               : invariantGraph().varId(outputVarNodeIds().at(i)).isView());

    outputVarIds.emplace_back(
        _intermediate.at(i) == propagation::NULL_ID
            ? invariantGraph().varId(outputVarNodeIds().at(i))
            : _intermediate.at(i));
  }

  solver().makeInvariant<propagation::GlobalCardinalityOpen>(
      solver(), std::move(outputVarIds), std::move(inputVarIds),
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
