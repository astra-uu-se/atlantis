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
    InvariantGraph& graph, VarNode& rowIdx, VarNode& colIdx,
    std::vector<std::vector<Int>>&& parMatrix, VarNode& output,
    const Int rowOffset, const Int colOffset, const bool isIntMatrix)
    : InvariantNode(graph, {output.ptr()}, {rowIdx.ptr(), colIdx.ptr()}),
      _parMatrix(std::move(parMatrix)),
      _rowOffset(rowOffset),
      _colOffset(colOffset),
      _isIntMatrix(isIntMatrix) {}

ArrayElement2dNode::ArrayElement2dNode(
    InvariantGraph& graph, VarNode& rowIdx, VarNode& colIdx,
    const std::vector<std::vector<bool>>& parMatrix, VarNode& output,
    const Int rowOffset, const Int colOffset)
    : ArrayElement2dNode(graph, rowIdx, colIdx, boolToViol(parMatrix), output,
                         rowOffset, colOffset, false) {}

void ArrayElement2dNode::init() {
  InvariantNode::init();
  assert(_isIntMatrix == outputVarNode(0).isIntVar());
}

void ArrayElement2dNode::postConstraint() {
  InvariantNode::postConstraint();
  if (outputVarNode(0).isIntVar()) {
    invariantGraph().constraintSolver().array_int_element2d(
        rowIdx().constraintVarId(),
        colIdx().constraintVarId(), _parMatrix,
        outputVarNode(0).constraintVarId(), _rowOffset, _colOffset);
  } else {
    invariantGraph().constraintSolver().array_bool_element2d(
        rowIdx().constraintVarId(),
        colIdx().constraintVarId(), violToBool(_parMatrix),
        outputVarNode(0).constraintVarId(), _rowOffset, _colOffset);
  }
}

void ArrayElement2dNode::updateState() {
  if (rowIdx().isFixed() && colIdx().isFixed()) {
    assert(outputVarNode(0).isFixed());
    setState(InvariantNodeState::SUBSUMED);
    return;
  }
  if (rowIdx().isFixed() || colIdx().isFixed()) {
    // this node should be replaced
    return;
  }

  if (outputVarNode(0).isFixed()) {
    const Int outVal = outputVarNode(0).lowerBound();
    const auto& rowDom = rowIdx().constDomain();
    const auto& colDom = colIdx().constDomain();
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
      for (const auto& vNode : staticInputVarNodes()) {
        vNode->tightenDomainType();
      }
      setState(InvariantNodeState::SUBSUMED);
    }
  }
}

bool ArrayElement2dNode::constrainsOutput(const VarNode&) const {
  std::vector<Int> values;
  values.reserve(_parMatrix.size() *
                 (_parMatrix.empty() ? 0 : _parMatrix.front().size()));
  for (auto rowIter = rowIdx().constDomain()->begin();
       rowIter != rowIdx().constDomain()->end(); ++rowIter) {
    const Int r = *rowIter - _rowOffset;
    if (r < 0) {
      continue;
      ;
    }
    if (static_cast<Int>(_parMatrix.size()) < r) {
      break;
    }
    for (auto colIter = colIdx().constDomain()->begin();
         colIter != colIdx().constDomain()->end(); ++colIter) {
      const Int c = *colIter - _colOffset;
      if (c < 0) {
        continue;
        ;
      }
      if (static_cast<Int>(_parMatrix[r].size()) < c) {
        break;
      }
      values.emplace_back(_parMatrix[r][c]);
    }
  }
  const SortedUniqueVector sortedVals(std::move(values));
  return !outputVarNode(0).constDomain()->contains(sortedVals);
}

bool ArrayElement2dNode::canBeReplaced() const {
  return state() == InvariantNodeState::ACTIVE &&
         (rowIdx().isFixed() || colIdx().isFixed());
}

bool ArrayElement2dNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  if (rowIdx().isFixed()) {
    const Int rowIndex = rowIdx().lowerBound() - _rowOffset;
    assert(rowIndex >= 0);
    assert(rowIndex < static_cast<Int>(_parMatrix.size()));

    invariantGraph().addInvariantNode(std::make_shared<ArrayElementNode>(
        invariantGraph(), std::move(_parMatrix.at(rowIndex)), colIdx(),
        outputVarNode(0), _colOffset));
    _parMatrix.clear();
    return true;
  }
  std::vector<Int> parMatrixCol;
  const Int colIndex = colIdx().lowerBound() - _colOffset;
  assert(colIndex >= 0);
  assert(colIndex < static_cast<Int>(_parMatrix.front().size()));
  parMatrixCol.reserve(_parMatrix.size());
  for (const std::vector<Int>& row : _parMatrix) {
    parMatrixCol.emplace_back(row.at(colIndex));
  }
  _parMatrix.clear();
  invariantGraph().addInvariantNode(std::make_shared<ArrayElementNode>(
      invariantGraph(), std::move(parMatrixCol), rowIdx(),
      outputVarNode(0), _rowOffset));
  return true;
}

void ArrayElement2dNode::registerOutputVars(propagation::SolverBase& solver,
                                            SolverMapping& mapping) const {
  if (!staticInputVarNodes().empty()) {
    makeSolverVar(outputVarNode(0), solver, mapping);
  }
  assert(std::ranges::all_of(
      outputVarNodes(), [&](const std::shared_ptr<VarNode>& vNode) {
        return mapping.solverId(vNode) != propagation::NULL_ID;
      }));
}

void ArrayElement2dNode::registerNode(propagation::SolverBase& solver,
                                      SolverMapping& mapping) const {
  if (staticInputVarNodes().empty()) {
    return;
  }
  assert(mapping.solverId(outputVarNode(0)) != propagation::NULL_ID);
  assert(mapping.solverId(outputVarNode(0)).isVar());

  solver.makeInvariant<propagation::Element2dConst>(
      solver, mapping.solverId(outputVarNode(0)),
      mapping.solverId(rowIdx()), mapping.solverId(colIdx()),
      std::vector<std::vector<Int>>{_parMatrix}, _rowOffset, _colOffset);
}

std::string ArrayElement2dNode::dotLangIdentifier() const {
  return "element2d";
}

}  // namespace atlantis::invariantgraph
