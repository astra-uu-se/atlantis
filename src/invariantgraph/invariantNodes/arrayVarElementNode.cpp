#include "atlantis/invariantgraph/invariantNodes/arrayVarElementNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayElementNode.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/invariants/elementVar.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

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

  idxNode.removeValuesBelow(_offset);
  idxNode.removeValuesAbove(
      _offset + static_cast<Int>(dynamicInputVarNodeIds().size()) - 1);

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
