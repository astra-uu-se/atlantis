#include "atlantis/invariantgraph/invariantNodes/arrayVarElementNode.hpp"

#include <algorithm>
#include <unordered_set>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayElementNode.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/invariants/elementVar.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

namespace {

bool domainsOverlap(const VarNode& a, const VarNode& b) {
  assert(a.isIntVar() == b.isIntVar());
  if (!a.isIntVar()) {
    return (a.inDomain(bool{true}) && b.inDomain(bool{true})) ||
           (a.inDomain(bool{false}) && b.inDomain(bool{false}));
  }
  const auto* smaller = &a;
  const auto* larger = &b;
  if (a.constDomain()->size() > b.constDomain()->size()) {
    std::swap(smaller, larger);
  }
  return std::any_of(smaller->constDomain()->begin(),
                     smaller->constDomain()->end(),
                     [&](const Int value) { return larger->inDomain(value); });
}

SortedUniqueVector commonDomain(const VarNode& a, const VarNode& b) {
  assert(a.isIntVar() == b.isIntVar());
  if (!a.isIntVar()) {
    std::vector<Int> values;
    values.reserve(2);
    if (a.inDomain(bool{true}) && b.inDomain(bool{true})) {
      values.emplace_back(0);
    }
    if (a.inDomain(bool{false}) && b.inDomain(bool{false})) {
      values.emplace_back(1);
    }
    return SortedUniqueVector(std::move(values));
  }
  const auto* smaller = &a;
  const auto* larger = &b;
  if (a.constDomain()->size() > b.constDomain()->size()) {
    std::swap(smaller, larger);
  }
  std::vector<Int> values;
  values.reserve(smaller->constDomain()->size());
  for (const Int value : *smaller->constDomain()) {
    if (larger->inDomain(value)) {
      values.emplace_back(value);
    }
  }
  return SortedUniqueVector(std::move(values));
}

}  // namespace

ArrayVarElementNode::ArrayVarElementNode(InvariantGraph& graph, VarNodeId idx,
                                         std::vector<VarNodeId>&& varVector,
                                         VarNodeId output, Int offset)
    : InvariantNode(graph, {output}, {idx}, std::move(varVector)),
      _offset(offset) {}

void ArrayVarElementNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(std::ranges::all_of(
      staticInputVarNodeIds().begin(), staticInputVarNodeIds().end(),
      [&](const VarNodeId node) {
        return invariantGraphConst().varNodeConst(node).isIntVar();
      }));
  assert(std::ranges::all_of(
      dynamicInputVarNodeIds().begin(), dynamicInputVarNodeIds().end(),
      [&](const VarNodeId node) {
        return invariantGraph()
                   .varNodeConst(outputVarNodeIds().front())
                   .isIntVar() ==
               invariantGraphConst().varNodeConst(node).isIntVar();
      }));
}

void ArrayVarElementNode::updateState() {
  VarNode& idxNode = invariantGraph().varNode(idx());
  VarNode& outputNode = invariantGraph().varNode(outputVarNodeIds().front());

  idxNode.removeValuesBelow(_offset);
  idxNode.removeValuesAbove(
      _offset + static_cast<Int>(dynamicInputVarNodeIds().size()) - 1);

  auto pruneIdxByOutputDomain = [&] {
    std::vector<Int> valuesToRemove;
    valuesToRemove.reserve(idxNode.constDomain()->size());
    for (const Int index : *idxNode.constDomain()) {
      const auto& inputNode = invariantGraphConst().varNodeConst(
          dynamicInputVarNodeIds().at(index - _offset));
      if (!domainsOverlap(inputNode, outputNode)) {
        valuesToRemove.emplace_back(index);
      }
    }
    if (!valuesToRemove.empty()) {
      idxNode.domain()->remove(SortedUniqueVector(std::move(valuesToRemove)));
    }
  };

  pruneIdxByOutputDomain();

  if (outputNode.isIntVar()) {
    std::unordered_set<Int> outputVals;
    outputVals.reserve(outputNode.constDomain()->size());
    for (const Int index : *idxNode.constDomain()) {
      const auto& inputNode = invariantGraphConst().varNodeConst(
          dynamicInputVarNodeIds().at(index - _offset));
      for (const Int value : *inputNode.constDomain()) {
        if (outputNode.inDomain(value)) {
          outputVals.emplace(value);
        }
      }
    }
    outputNode.domain()->removeAllValuesExcept(SortedUniqueVector(
        std::vector<Int>(outputVals.begin(), outputVals.end())));
  } else {
    const bool canHoldTrue = std::any_of(
        idxNode.constDomain()->begin(), idxNode.constDomain()->end(),
        [&](const Int index) {
          return invariantGraphConst()
              .varNodeConst(dynamicInputVarNodeIds().at(index - _offset))
              .inDomain(bool{true});
        });
    const bool canHoldFalse = std::any_of(
        idxNode.constDomain()->begin(), idxNode.constDomain()->end(),
        [&](const Int index) {
          return invariantGraphConst()
              .varNodeConst(dynamicInputVarNodeIds().at(index - _offset))
              .inDomain(bool{false});
        });
    if (!canHoldTrue) {
      outputNode.fixToValue(bool{false});
    } else if (!canHoldFalse) {
      outputNode.fixToValue(bool{true});
    }
  }

  pruneIdxByOutputDomain();

  if (idxNode.isFixed()) {
    auto& selectedNode = invariantGraph().varNode(
        dynamicInputVarNodeIds().at(idxNode.lowerBound() - _offset));
    const SortedUniqueVector overlap = commonDomain(selectedNode, outputNode);
    selectedNode.domain()->removeAllValuesExcept(overlap);
    outputNode.domain()->removeAllValuesExcept(overlap);
  }

  const Int overflow = _offset +
                       static_cast<Int>(dynamicInputVarNodeIds().size()) - 1 -
                       idxNode.upperBound();

  const Int underflow = idxNode.lowerBound() - _offset;

  if (overflow <= 0 && underflow <= 0) {
    return;
  }

  std::vector<VarNodeId> varNodeIdsToRemove;
  varNodeIdsToRemove.reserve(overflow + underflow);

  for (Int i = static_cast<Int>(dynamicInputVarNodeIds().size()) - overflow;
       i < static_cast<Int>(dynamicInputVarNodeIds().size()); ++i) {
    varNodeIdsToRemove.emplace_back(dynamicInputVarNodeIds().at(i));
  }

  for (Int i = 0; i < underflow; ++i) {
    varNodeIdsToRemove.emplace_back(dynamicInputVarNodeIds().at(i));
  }

  _offset += underflow;

  _dynamicInputVarNodeIds.erase(_dynamicInputVarNodeIds.end() - overflow,
                                _dynamicInputVarNodeIds.end());
  _dynamicInputVarNodeIds.erase(_dynamicInputVarNodeIds.begin(),
                                _dynamicInputVarNodeIds.begin() + underflow);

  for (const auto& vId : varNodeIdsToRemove) {
    if (std::ranges::none_of(dynamicInputVarNodeIds().begin(),
                             dynamicInputVarNodeIds().end(),
                             [&](const VarNodeId dId) { return dId == vId; })) {
      removeDynamicInputVarNode(vId);
    }
  }
}

bool ArrayVarElementNode::canBeReplaced() const {
  if (state() != InvariantNodeState::ACTIVE) {
    return false;
  }
  if (invariantGraphConst().varNodeConst(idx()).isFixed()) {
    return true;
  }
  const auto& dom = invariantGraphConst().varNodeConst(idx()).constDomain();
  return std::all_of(dom->begin(), dom->end(), [&](const Int val) {
    if (val < _offset ||
        val >= _offset + static_cast<Int>(dynamicInputVarNodeIds().size())) {
      return true;
    }
    return invariantGraphConst()
        .varNodeConst(dynamicInputVarNodeIds().at(val - _offset))
        .isFixed();
  });
}

bool ArrayVarElementNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  const auto& idxNode = invariantGraphConst().varNodeConst(idx());
  if (invariantGraphConst().varNodeConst(idx()).isFixed()) {
    const VarNodeId input =
        dynamicInputVarNodeIds().at(idxNode.lowerBound() - _offset);
    invariantGraph().replaceVarNode(outputVarNodeIds().front(), input);
    return true;
  }
  const Int defVal = invariantGraph()
                         .varNodeConst(idxNode.lowerBound() - _offset)
                         .lowerBound();
  std::vector<Int> parameters(dynamicInputVarNodeIds().size(), defVal);
  for (const Int idxVal : *idxNode.constDomain()) {
    const Int index = idxVal - _offset;
    parameters[index] = invariantGraph()
                            .varNodeConst(dynamicInputVarNodeIds().at(index))
                            .lowerBound();
  }
  invariantGraph().addInvariantNode(std::make_shared<ArrayElementNode>(
      invariantGraph(), std::move(parameters), idx(),
      outputVarNodeIds().front(), _offset));
  return true;
}

void ArrayVarElementNode::registerOutputVars(propagation::SolverBase& solver,
                                             SolverMapping& mapping) const {
  makeSolverVar(outputVarNodeIds().front(), _offset, solver, mapping);
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

void ArrayVarElementNode::registerNode(propagation::SolverBase& solver,
                                       SolverMapping& mapping) const {
  std::vector<propagation::VarViewId> varVector;
  std::ranges::transform(dynamicInputVarNodeIds(),
                         std::back_inserter(varVector),
                         [&](auto nId) { return mapping.solverId(nId); });

  assert(mapping.solverId(outputVarNodeIds().front()) != propagation::NULL_ID);
  assert(mapping.solverId(outputVarNodeIds().front()).isVar());

  solver.makeInvariant<propagation::ElementVar>(
      solver, mapping.solverId(outputVarNodeIds().front()),
      mapping.solverId(idx()), std::move(varVector), _offset);
}

std::string ArrayVarElementNode::dotLangIdentifier() const {
  return "var_element";
}

}  // namespace atlantis::invariantgraph
