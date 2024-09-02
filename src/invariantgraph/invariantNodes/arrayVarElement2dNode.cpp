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
    VarNodeId idx1, VarNodeId idx2, std::vector<VarNodeId>&& flatVarMatrix,
    VarNodeId output, size_t numRows, Int offset1, Int offset2)
    : InvariantNode({output}, {idx1, idx2}, std::move(flatVarMatrix)),
      _numRows(numRows),
      _offset1(offset1),
      _offset2(offset2) {}

ArrayVarElement2dNode::ArrayVarElement2dNode(
    VarNodeId idx1, VarNodeId idx2,
    std::vector<std::vector<VarNodeId>>&& varMatrix, VarNodeId output,
    Int offset1, Int offset2)
    : ArrayVarElement2dNode(idx1, idx2, flatten(varMatrix), output,
                            varMatrix.size(), offset1, offset2) {}

void ArrayVarElement2dNode::init(const InvariantNodeId& id) {
  InvariantNode::init(id);
  assert(std::all_of(staticInputVarNodeIds().begin(),
                     staticInputVarNodeIds().end(), [&](const VarNodeId& node) {
                       return graph.varNodeConst(node).isIntVar();
                     }));
  assert(std::all_of(
      dynamicInputVarNodeIds().begin(), dynamicInputVarNodeIds().end(),
      [&](const VarNodeId& node) {
        return graph.varNodeConst(outputVarNodeIds().front()).isIntVar() ==
               graph.varNodeConst(node).isIntVar();
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
  VarNode& idx1Node = graph.varNode(idx1());
  // idx1Node.removeValuesBelow(_offset1);
  // idx1Node.removeValuesAbove(_offset1 + static_cast<Int>(_numRows) - 1);

  VarNode& idx2Node = graph.varNode(idx2());
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
      if (graph.varNodeConst(_dynamicInputVarNodeIds.at(pos)).isFixed()) {
        continue;
      }

      varNodeIdsToRemove.emplace_back(_dynamicInputVarNodeIds.at(pos));

      if (placeholder == NULL_NODE_ID) {
        placeholder = graph.varNodeConst(outputVarNodeIds().front()).isIntVar()
                          ? graph.retrieveIntVarNode(0)
                          : graph.retrieveBoolVarNode(false);
      }
      _dynamicInputVarNodeIds.at(pos) = placeholder;
    }
  }

  if (placeholder != NULL_NODE_ID &&
      std::none_of(
          graph.varNodeConst(placeholder).dynamicInputTo().begin(),
          graph.varNodeConst(placeholder).dynamicInputTo().end(),
          [&](const InvariantNodeId& invId) { return invId == id(); })) {
    graph.varNode(placeholder).markAsInputFor(id(), false);
  }

  for (const auto& vId : varNodeIdsToRemove) {
    if (std::none_of(dynamicInputVarNodeIds().begin(),
                     dynamicInputVarNodeIds().end(),
                     [&](const VarNodeId& dId) { return dId == vId; })) {
      removeDynamicInputVarNode(graph.varNode(vId));
    }
  }
}

void ArrayVarElement2dNode::registerOutputVars(
    InvariantGraph& graph, propagation::SolverBase& solver) {
  makeSolverVar(solver, graph.varNode(outputVarNodeIds().front()));
  assert(std::all_of(outputVarNodeIds().begin(), outputVarNodeIds().end(),
                     [&](const VarNodeId& vId) {
                       return graph.varNodeConst(vId).varId() !=
                              propagation::NULL_ID;
                     }));
}

bool ArrayVarElement2dNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         (graph.varNodeConst(idx1()).isFixed() ||
          graph.varNodeConst(idx2()).isFixed());
}

bool ArrayVarElement2dNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  auto& idx1Node = graph.varNode(idx1());
  auto& idx2Node = graph.varNode(idx2());
  assert(idx1Node.isFixed() || idx2Node.isFixed());
  if (idx1Node.isFixed() && idx2Node.isFixed()) {
    const VarNodeId input = at(idx1Node.lowerBound(), idx2Node.lowerBound());
    graph.replaceVarNode(outputVarNodeIds().front(), input);
  } else if (idx1Node.isFixed()) {
    std::vector<VarNodeId> column;
    column.reserve(_numRows);
    for (Int i = 0; i < static_cast<Int>(_numRows); ++i) {
      column.emplace_back(at(idx1Node.lowerBound(), i + _offset2));
    }
    graph.addInvariantNode(std::make_shared<ArrayVarElementNode>(
        graph, idx2(), std::move(column), outputVarNodeIds().front(),
        _offset2));
  } else {
    assert(idx2Node.isFixed());
    const Int numCols = dynamicInputVarNodeIds().size() / _numRows;
    std::vector<VarNodeId> row;
    row.reserve(numCols);
    for (Int i = 0; i < numCols; ++i) {
      row.emplace_back(at(i + _offset1, idx2Node.lowerBound()));
    }
    graph.addInvariantNode(std::make_shared<ArrayVarElementNode>(
        graph, idx1(), std::move(row), outputVarNodeIds().front(), _offset1));
  }
  return true;
}

void ArrayVarElement2dNode::registerNode() {
  const size_t numCols = dynamicInputVarNodeIds().size() / _numRows;
  std::vector<std::vector<propagation::VarId>> varMatrix(
      _numRows, std::vector<propagation::VarId>(numCols));
  for (size_t i = 0; i < _numRows; ++i) {
    for (size_t j = 0; j < numCols; ++j) {
      varMatrix.at(i).at(j) =
          graph.varNode(dynamicInputVarNodeIds().at(i * numCols + j)).varId();
    }
  }

  assert(graph.varId(outputVarNodeIds().front()) != propagation::NULL_ID);
  solver.makeInvariant<propagation::Element2dVar>(
      solver, graph.varId(outputVarNodeIds().front()), graph.varId(idx1()),
      graph.varId(idx2()), std::move(varMatrix), _offset1, _offset2);
}

}  // namespace atlantis::invariantgraph
