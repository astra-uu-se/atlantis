#include "atlantis/invariantgraph/invariantNodes/arrayVarElement2dNode.hpp"

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayVarElement2dNode.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayVarElementNode.hpp"
#include "atlantis/propagation/invariants/element2dVar.hpp"

namespace atlantis::invariantgraph {

static std::vector<VarNodeId> flatten(
    const std::vector<std::vector<VarNodeId>>& varMatrix) {
  std::vector<VarNodeId> flatVarMatrix;
  flatVarMatrix.reserve(varMatrix.size() * varMatrix.front().size());
  for (const auto& varVector : varMatrix) {
    flatVarMatrix.insert(flatVarMatrix.end(), varVector.begin(),
                         varVector.end());
  }
  return flatVarMatrix;
}

ArrayVarElement2dNode::ArrayVarElement2dNode(
    IInvariantGraph& graph, VarNodeId idx1, VarNodeId idx2,
    std::vector<VarNodeId>&& flatVarMatrix, VarNodeId output, size_t numRows,
    Int offset1, Int offset2)
    : InvariantNode(graph, {output}, {idx1, idx2}, std::move(flatVarMatrix)),
      _numRows(numRows),
      _offset1(offset1),
      _offset2(offset2) {}

ArrayVarElement2dNode::ArrayVarElement2dNode(
    IInvariantGraph& graph, VarNodeId idx1, VarNodeId idx2,
    std::vector<std::vector<VarNodeId>>&& varMatrix, VarNodeId output,
    Int offset1, Int offset2)
    : ArrayVarElement2dNode(graph, idx1, idx2, flatten(varMatrix), output,
                            varMatrix.size(), offset1, offset2) {}

void ArrayVarElement2dNode::init(InvariantNodeId id) {
  InvariantNode::init(id);
  assert(
      std::all_of(staticInputVarNodeIds().begin(),
                  staticInputVarNodeIds().end(), [&](const VarNodeId node) {
                    return invariantGraphConst().varNodeConst(node).isIntVar();
                  }));
  assert(
      std::all_of(dynamicInputVarNodeIds().begin(),
                  dynamicInputVarNodeIds().end(), [&](const VarNodeId node) {
                    return invariantGraph()
                               .varNodeConst(outputVarNodeIds().front())
                               .isIntVar() ==
                           invariantGraphConst().varNodeConst(node).isIntVar();
                  }));
}

VarNodeId ArrayVarElement2dNode::at(Int row, Int col) {
  assert(row >= _offset1 && col >= _offset2);
  const size_t r = static_cast<size_t>(row - _offset1);
  assert(r < _numRows);
  const size_t c = static_cast<size_t>(col - _offset2);
  assert(col >= 0);
  const size_t pos = r * _numRows + c;
  return dynamicInputVarNodeIds().at(pos);
}

void ArrayVarElement2dNode::updateState() {
  VarNode& idx1Node = invariantGraph().varNode(idx1());
  // idx1Node.removeValuesBelow(_offset1);
  // idx1Node.removeValuesAbove(_offset1 + static_cast<Int>(_numRows) - 1);

  VarNode& idx2Node = invariantGraph().varNode(idx2());
  // idx2Node.removeValuesBelow(_offset2);
  // idx2Node.removeValuesAbove(_offset2 + static_cast<Int>(numCols()) - 1);

  std::vector<VarNodeId> varNodeIdsToRemove;
  varNodeIdsToRemove.reserve(dynamicInputVarNodeIds().size());

  VarNodeId placeholder{NULL_NODE_ID};

  for (Int row = _offset1; row < _offset1 + static_cast<Int>(_numRows); ++row) {
    const bool inDom1 = idx1Node.inDomain(row);
    for (Int col = _offset2; col < _offset2 + static_cast<Int>(numCols());
         ++col) {
      if (inDom1 && idx2Node.inDomain(col)) {
        continue;
      }
      assert(row >= _offset1 && col >= _offset2);
      const size_t r = static_cast<size_t>(row - _offset1);
      assert(r < _numRows);
      const size_t c = static_cast<size_t>(col - _offset2);
      assert(col >= 0);
      const size_t pos = r * _numRows + c;
      if (invariantGraph()
              .varNodeConst(_dynamicInputVarNodeIds.at(pos))
              .isFixed()) {
        continue;
      }

      varNodeIdsToRemove.emplace_back(_dynamicInputVarNodeIds.at(pos));

      if (placeholder == NULL_NODE_ID) {
        placeholder = invariantGraphConst()
                              .varNodeConst(outputVarNodeIds().front())
                              .isIntVar()
                          ? invariantGraph().retrieveIntVarNode(0)
                          : invariantGraph().retrieveBoolVarNode(false);
      }
      _dynamicInputVarNodeIds.at(pos) = placeholder;
    }
  }

  if (placeholder != NULL_NODE_ID &&
      std::none_of(
          invariantGraphConst()
              .varNodeConst(placeholder)
              .dynamicInputTo()
              .begin(),
          invariantGraphConst()
              .varNodeConst(placeholder)
              .dynamicInputTo()
              .end(),
          [&](const InvariantNodeId& invId) { return invId == id(); })) {
    invariantGraph().varNode(placeholder).markAsInputFor(id(), false);
  }

  for (const auto& vId : varNodeIdsToRemove) {
    if (std::none_of(dynamicInputVarNodeIds().begin(),
                     dynamicInputVarNodeIds().end(),
                     [&](const VarNodeId dId) { return dId == vId; })) {
      removeDynamicInputVarNode(vId);
    }
  }
}

void ArrayVarElement2dNode::registerOutputVars() {
  makeSolverVar(outputVarNodeIds().front());
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId vId) {
                       return invariantGraphConst().varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

bool ArrayVarElement2dNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         (invariantGraphConst().varNodeConst(idx1()).isFixed() ||
          invariantGraphConst().varNodeConst(idx2()).isFixed());
}

bool ArrayVarElement2dNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  auto& idx1Node = invariantGraph().varNode(idx1());
  auto& idx2Node = invariantGraph().varNode(idx2());
  assert(idx1Node.isFixed() || idx2Node.isFixed());
  if (idx1Node.isFixed() && idx2Node.isFixed()) {
    const VarNodeId input = at(idx1Node.lowerBound(), idx2Node.lowerBound());
    invariantGraph().replaceVarNode(outputVarNodeIds().front(), input);
  } else if (idx1Node.isFixed()) {
    std::vector<VarNodeId> column;
    column.reserve(_numRows);
    for (Int i = 0; i < static_cast<Int>(_numRows); ++i) {
      column.emplace_back(at(idx1Node.lowerBound(), i + _offset2));
    }
    invariantGraph().addInvariantNode(std::make_shared<ArrayVarElementNode>(
        invariantGraph(), idx2(), std::move(column), outputVarNodeIds().front(),
        _offset2));
  } else {
    assert(idx2Node.isFixed());
    const Int numCols = dynamicInputVarNodeIds().size() / _numRows;
    std::vector<VarNodeId> row;
    row.reserve(numCols);
    for (Int i = 0; i < numCols; ++i) {
      row.emplace_back(at(i + _offset1, idx2Node.lowerBound()));
    }
    invariantGraph().addInvariantNode(std::make_shared<ArrayVarElementNode>(
        invariantGraph(), idx1(), std::move(row), outputVarNodeIds().front(),
        _offset1));
  }
  return true;
}

void ArrayVarElement2dNode::registerNode() {
  const size_t numCols = dynamicInputVarNodeIds().size() / _numRows;
  std::vector<std::vector<propagation::VarViewId>> varMatrix(
      _numRows, std::vector<propagation::VarViewId>{});
  for (size_t i = 0; i < _numRows; ++i) {
    varMatrix.at(i).reserve(numCols);
    for (size_t j = 0; j < numCols; ++j) {
      varMatrix.at(i).emplace_back(
          invariantGraph()
              .varNode(dynamicInputVarNodeIds().at(i * numCols + j))
              .varId());
    }
  }

  assert(invariantGraph().varId(outputVarNodeIds().front()) !=
         propagation::NULL_ID);
  assert(invariantGraph().varId(outputVarNodeIds().front()).isVar());
  solver().makeInvariant<propagation::Element2dVar>(
      solver(), invariantGraph().varId(outputVarNodeIds().front()),
      invariantGraph().varId(idx1()), invariantGraph().varId(idx2()),
      std::move(varMatrix), _offset1, _offset2);
}

std::string ArrayVarElement2dNode::dotLangIdentifier() const {
  return "var_element2d";
}

}  // namespace atlantis::invariantgraph
