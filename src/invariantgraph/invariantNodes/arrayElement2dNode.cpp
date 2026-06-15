#include "atlantis/invariantgraph/invariantNodes/arrayElement2dNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayElementNode.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/invariants/element2dConst.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

ArrayElement2dNode::ArrayElement2dNode(
    InvariantGraph& graph, const VarNodeId rowIdx, const VarNodeId colIdx,
    std::vector<std::vector<Int>>&& parMatrix, const VarNodeId output,
    const Int rowOffset, const Int colOffset, const bool isIntMatrix)
    : InvariantNode(graph, {output}, {rowIdx, colIdx}),
      _parMatrix(std::move(parMatrix)),
      _rowOffset(rowOffset),
      _colOffset(colOffset),
      _isIntMatrix(isIntMatrix) {}

ArrayElement2dNode::ArrayElement2dNode(
    InvariantGraph& graph, const VarNodeId rowIdx, const VarNodeId colIdx,
    const std::vector<std::vector<bool>>& parMatrix, const VarNodeId output,
    const Int rowOffset, const Int colOffset)
    : ArrayElement2dNode(graph, rowIdx, colIdx, boolToViol(parMatrix), output,
                         rowOffset, colOffset, false) {}
void ArrayElement2dNode::init(const InvariantNodeId id) {
  InvariantNode::init(id);
  assert(_isIntMatrix == outputVarNode(0).isIntVar());
}

void ArrayElement2dNode::postConstraint() {
  InvariantNode::postConstraint();
  const auto& outputNode = outputVarNodeConst(0);
  if (outputNode.isIntVar()) {
    invariantGraph().constraintSolver().array_int_element2d(
        varNodeConst(rowIdx()).constraintVarId(),
        varNodeConst(colIdx()).constraintVarId(), _parMatrix,
        outputNode.constraintVarId(), _rowOffset, _colOffset);
  } else {
    invariantGraph().constraintSolver().array_bool_element2d(
        varNodeConst(rowIdx()).constraintVarId(),
        varNodeConst(colIdx()).constraintVarId(), violToBool(_parMatrix),
        outputNode.constraintVarId(), _rowOffset, _colOffset);
  }
}

void ArrayElement2dNode::updateState() {
  if (varNode(rowIdx()).isFixed() && varNode(colIdx()).isFixed()) {
    assert(outputVarNodeConst(0).isFixed());
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (varNode(rowIdx()).isFixed() || varNode(colIdx()).isFixed()) {
    // this node should be replaced
    return;
  }

  if (outputVarNodeConst(0).isFixed()) {
    const Int outVal = outputVarNodeConst(0).lowerBound();
    const auto& rowDom = varNodeConst(rowIdx()).constDomain();
    const auto& colDom = varNodeConst(colIdx()).constDomain();
    bool allSatisfying = true;
    for (auto rowIter = rowDom->begin(); rowIter != rowDom->end(); ++rowIter) {
      const Int row = *rowIter - _rowOffset;
      assert(0 <= row && row < static_cast<Int>(_parMatrix.size()));
      for (auto colIter = colDom->begin(); colIter != colDom->end();
           ++colIter) {
        const Int col = *colIter - _colOffset;
        assert(0 <= col && col < static_cast<Int>(_parMatrix.at(row).size()));
        if (_parMatrix[row][col] != outVal) {
          allSatisfying = false;
          break;
        }
      }
    }
    if (allSatisfying) {
      for (const auto vId : staticInputVarNodeIds()) {
        varNode(vId).tightenDomainType();
      }
      setState(InvariantNodeState::SUBSUMED);
    }
  }
}

bool ArrayElement2dNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         (varNodeConst(rowIdx()).isFixed() || varNodeConst(colIdx()).isFixed());
}

bool ArrayElement2dNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (varNode(rowIdx()).isFixed()) {
    const Int rowIndex = varNode(rowIdx()).lowerBound() - _rowOffset;
    assert(rowIndex >= 0);
    assert(rowIndex < static_cast<Int>(_parMatrix.size()));

    invariantGraph().addInvariantNode(std::make_shared<ArrayElementNode>(
        invariantGraph(), std::move(_parMatrix.at(rowIndex)), colIdx(),
        outputVarNodeIds().front(), _colOffset));
    _parMatrix.clear();
    return true;
  }
  std::vector<Int> parMatrixCol;
  const Int colIndex = varNode(colIdx()).lowerBound() - _colOffset;
  assert(colIndex >= 0);
  assert(colIndex < static_cast<Int>(_parMatrix.front().size()));
  parMatrixCol.reserve(_parMatrix.size());
  for (const std::vector<Int>& row : _parMatrix) {
    parMatrixCol.emplace_back(row.at(colIndex));
  }
  _parMatrix.clear();
  invariantGraph().addInvariantNode(std::make_shared<ArrayElementNode>(
      invariantGraph(), std::move(parMatrixCol), rowIdx(),
      outputVarNodeIds().front(), _rowOffset));
  return true;
}

void ArrayElement2dNode::registerOutputVars(propagation::SolverBase& solver,
                                            SolverMapping& mapping) const {
  if (!staticInputVarNodeIds().empty()) {
    makeSolverVar(outputVarNodeIds().front(), solver, mapping);
  }
  assert(std::ranges::all_of(outputVarNodeIds(), [&](const VarNodeId vId) {
    return mapping.solverId(vId) != propagation::NULL_ID;
  }));
}

void ArrayElement2dNode::registerNode(propagation::SolverBase& solver,
                                      SolverMapping& mapping) const {
  if (staticInputVarNodeIds().empty()) {
    return;
  }
  assert(mapping.solverId(outputVarNodeIds().front()) != propagation::NULL_ID);
  assert(mapping.solverId(outputVarNodeIds().front()).isVar());

  solver.makeInvariant<propagation::Element2dConst>(
      solver, mapping.solverId(outputVarNodeIds().front()),
      mapping.solverId(rowIdx()), mapping.solverId(colIdx()),
      std::vector<std::vector<Int>>{_parMatrix}, _rowOffset, _colOffset);
}

std::string ArrayElement2dNode::dotLangIdentifier() const {
  return "element2d";
}

}  // namespace atlantis::invariantgraph
