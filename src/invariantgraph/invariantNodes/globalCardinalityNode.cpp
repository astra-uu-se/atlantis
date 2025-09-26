#include "atlantis/invariantgraph/invariantNodes/globalCardinalityNode.hpp"

#include <algorithm>
#include <numeric>
#include <stack>
#include <utility>
#include <vector>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNodes/intCountNode.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/invariantgraph/violationInvariantNodes/intAllEqualNode.hpp"
#include "atlantis/propagation/invariants/globalCardinalityOpen.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/views/equalConst.hpp"
#include "atlantis/propagation/views/intOffsetView.hpp"

namespace atlantis::invariantgraph {

GlobalCardinalityNode::GlobalCardinalityNode(InvariantGraph& graph,
                                             std::vector<VarNodeId>&& inputs,
                                             std::vector<Int>&& cover,
                                             std::vector<VarNodeId>&& counts)
    : InvariantNode(graph, std::move(counts), std::move(inputs)),
      _cover(std::move(cover)),
      _countOffsets(_cover.size(), 0) {
  assert(_cover.size() == outputVarNodeIds().size());
  if (_cover.empty()) {
    setState(InvariantNodeState::SUBSUMED);
  }
}

void GlobalCardinalityNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).isIntVar();
      }));
  assert(std::ranges::all_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const VarNodeId vId) {
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
  for (Int i = 0; i < static_cast<Int>(_cover.size()); i++) {
    for (Int j = static_cast<Int>(_cover.size()) - 1; j > i; --j) {
      if (_cover[i] == _cover[j]) {
        _cover.erase(_cover.begin() + j);
        const VarNodeId duplicate = outputVarNodeIds()[j];
        removeOutputAtIndex(j);
        invariantGraph().replaceVarNode(duplicate, outputVarNodeIds()[i]);
      }
    }
  }

  std::vector<std::vector<size_t>> supportedInputs(_cover.size());
  std::vector<std::vector<size_t>> supportedCovers(
      staticInputVarNodeIds().size());
  for (size_t inputIndex = 0; inputIndex < staticInputVarNodeIds().size();
       inputIndex++) {
    const auto& var =
        invariantGraphConst().varNodeConst(staticInputVarNodeIds()[inputIndex]);
    if (var.isFixed()) {
      for (size_t coverIndex = 0; coverIndex < _cover.size(); ++coverIndex) {
        if (var.lowerBound() == _cover[coverIndex]) {
          ++_countOffsets[coverIndex];
        }
      }
    } else {
      for (size_t coverIndex = 0; coverIndex < _cover.size(); ++coverIndex) {
        if (var.inDomain(_cover[coverIndex])) {
          supportedInputs[coverIndex].emplace_back(inputIndex);
          supportedCovers[inputIndex].emplace_back(coverIndex);
        }
      }
    }
  }
  std::vector<bool> onStack(_cover.size(), true);
  std::stack<size_t> stack;
  for (size_t coverIndex = 0; coverIndex < _cover.size(); ++coverIndex) {
    stack.push(coverIndex);
  }

  while (!stack.empty()) {
    const size_t coverIndex = stack.top();
    stack.pop();
    auto& outVar = invariantGraph().varNode(outputVarNodeIds()[coverIndex]);
    const Int lb = _countOffsets[coverIndex];
    const Int ub = _countOffsets[coverIndex] +
                   static_cast<Int>(supportedInputs[coverIndex].size());
    outVar.removeValuesBelow(lb);
    outVar.removeValuesAbove(ub);
    if (outVar.lowerBound() == ub) {
      for (const size_t inputIndex : supportedInputs[coverIndex]) {
        auto& vNode =
            invariantGraph().varNode(staticInputVarNodeIds()[inputIndex]);
        vNode.fixToValue(_cover[coverIndex]);
        for (const size_t otherCover : supportedCovers[inputIndex]) {
          if (otherCover != coverIndex) {
            removeFirstOccurrence(supportedInputs[otherCover], inputIndex);
            if (!onStack[otherCover]) {
              stack.push(otherCover);
              onStack[otherCover] = true;
            }
          }
        }
        supportedInputs[coverIndex].clear();
      }
    } else if (outVar.upperBound() == lb) {
      for (const size_t inputIndex : supportedInputs[coverIndex]) {
        auto& vNode =
            invariantGraph().varNode(staticInputVarNodeIds()[inputIndex]);
        vNode.removeValue(_cover[coverIndex]);
        removeFirstOccurrence(supportedCovers[inputIndex], coverIndex);
        if (vNode.isFixed() && !supportedCovers[inputIndex].empty()) {
          assert(supportedCovers[inputIndex].size() == 1);
          const size_t otherCover = supportedCovers[inputIndex].front();
          assert(otherCover != coverIndex);
          ++_countOffsets[otherCover];
          removeFirstOccurrence(supportedInputs[otherCover], inputIndex);
          if (!onStack[otherCover]) {
            stack.push(otherCover);
            onStack[otherCover] = true;
          }
        }
      }
      supportedInputs[coverIndex].clear();
    }
    onStack[coverIndex] = false;
  }

  std::vector<VarNodeId> outputsToRemove;
  outputsToRemove.reserve(_cover.size());
  for (Int i = static_cast<Int>(_cover.size()) - 1; i >= 0; --i) {
    if (supportedInputs[i].empty()) {
      outputsToRemove.emplace_back(outputVarNodeIds()[i]);
      _countOffsets.erase(_countOffsets.begin() + i);
      _cover.erase(_cover.begin() + i);
    }
  }
  for (const auto& output : outputsToRemove) {
    removeOutputVarNode(output);
  }
  std::vector<VarNodeId> inputsToRemove;
  inputsToRemove.reserve(_cover.size());
  for (Int i = static_cast<Int>(staticInputVarNodeIds().size()) - 1; i >= 0;
       --i) {
    if (supportedCovers[i].empty()) {
      inputsToRemove.emplace_back(staticInputVarNodeIds()[i]);
    }
  }
  for (const auto& input : inputsToRemove) {
    removeStaticInputVarNode(input);
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
      _cover.front(), outputVarNodeIds().front(), _countOffsets.front()));
  return true;
}

void GlobalCardinalityNode::registerOutputVars(propagation::SolverBase& solver,
                                               SolverMapping& mapping) const {
  for (size_t i = 0; i < _cover.size(); ++i) {
    const bool isDuplicate = std::ranges::any_of(
        outputVarNodeIds().begin(),
        outputVarNodeIds().begin() + static_cast<Int>(i),
        [&](const VarNodeId vId) { return vId == outputVarNodeIds().at(i); });

    assert(
        !isDuplicate ||
        invariantGraphConst().varNodeConst(outputVarNodeIds().at(i)).isFixed());

    if (isDuplicate) {
      assert(mapping.solverId(outputVarNodeIds().at(i)) !=
             propagation::NULL_ID);
      mapping.setIntermediateId(id(), i, solver.makeIntVar(0, 0, 0));
      solver.makeIntView<propagation::EqualConst>(
          solver, mapping.intermediateId(id(), i),
          invariantGraphConst()
                  .varNodeConst(outputVarNodeIds().at(i))
                  .lowerBound() -
              _countOffsets[i]);
    } else if (_countOffsets[i] == 0) {
      assert(mapping.solverId(outputVarNodeIds().at(i)) ==
             propagation::NULL_ID);
      makeSolverVar(outputVarNodeIds().at(i), solver, mapping);
    } else {
      assert(mapping.solverId(outputVarNodeIds().at(i)) ==
             propagation::NULL_ID);
      mapping.setIntermediateId(id(), i, solver.makeIntVar(0, 0, 0));
      mapping.setSolverId(
          outputVarNodeIds().at(i),
          solver.makeIntView<propagation::IntOffsetView>(
              solver, mapping.intermediateId(id(), i), _countOffsets[i]));
    }
  }
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void GlobalCardinalityNode::registerNode(propagation::SolverBase& solver,
                                         SolverMapping& mapping) const {
  std::vector<propagation::VarViewId> inputVarIds;
  std::ranges::transform(staticInputVarNodeIds(),
                         std::back_inserter(inputVarIds),
                         [&](const auto& id) { return mapping.solverId(id); });

  std::vector<propagation::VarViewId> outputVarIds;
  outputVarIds.reserve(outputVarNodeIds().size());
  for (size_t i = 0; i < _cover.size(); ++i) {
    assert(mapping.intermediateId(id(), i) == propagation::NULL_ID ||
           mapping.intermediateId(id(), i).isVar());

    outputVarIds.emplace_back(mapping.intermediateId(id(), i) ==
                                      propagation::NULL_ID
                                  ? mapping.solverId(outputVarNodeIds().at(i))
                                  : mapping.intermediateId(id(), i));
  }

  solver.makeInvariant<propagation::GlobalCardinalityOpen>(
      solver, std::move(outputVarIds), std::move(inputVarIds),
      std::vector<Int>(_cover));
}

std::string GlobalCardinalityNode::dotLangIdentifier() const {
  std::string s{"global_cardinality ["};
  for (size_t i = 0; i < _cover.size(); ++i) {
    s += std::to_string(_cover.at(i));
    if (i < _cover.size() - 1) {
      s += ", ";
    }
  }
  s += ']';
  return s;
}

}  // namespace atlantis::invariantgraph
