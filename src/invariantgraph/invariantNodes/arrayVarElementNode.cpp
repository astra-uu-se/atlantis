#include "atlantis/invariantgraph/invariantNodes/arrayVarElementNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/invariants/elementVar.hpp"
#include "atlantis/propagation/solverBase.hpp"

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
                       static_cast<Int>(dynamicInputVarNodeIds().size()) -
                       idxNode.upperBound() - 1;

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
  return state() == InvariantNodeState::ACTIVE &&
         invariantGraphConst().varNodeConst(idx()).isFixed();
}

bool ArrayVarElementNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  const auto& idxNode = invariantGraph().varNode(idx());
  const VarNodeId input =
      dynamicInputVarNodeIds().at(idxNode.lowerBound() - _offset);
  invariantGraph().replaceVarNode(outputVarNodeIds().front(), input);
  return true;
}

void ArrayVarElementNode::registerOutputVars() {
  makeSolverVar(outputVarNodeIds().front(), _offset);
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return invariantGraphConst().varNodeConst(vId).varId() !=
               propagation::NULL_ID;
      }));
}

void ArrayVarElementNode::registerNode() {
  std::vector<propagation::VarViewId> varVector;
  std::ranges::transform(
      dynamicInputVarNodeIds(), std::back_inserter(varVector),
      [&](auto node) { return invariantGraph().varId(node); });

  assert(invariantGraph().varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);
  assert(invariantGraph().varId(outputVarNodeIds().front()).isVar());

  solver().makeInvariant<propagation::ElementVar>(
      solver(), invariantGraph().varId(outputVarNodeIds().front()),
      invariantGraph().varId(idx()), std::move(varVector), _offset);
}

std::string ArrayVarElementNode::dotLangIdentifier() const {
  return "var_element";
}

}  // namespace atlantis::invariantgraph
