#include "atlantis/invariantgraph/invariantNodes/arrayVarElement2dNode.hpp"

#include <algorithm>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/constraintSolver.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayElement2dNode.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayElementNode.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayVarElementNode.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/invariants/element2dVar.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

static std::vector<std::shared_ptr<VarNode>> flatten(
    const std::vector<std::vector<std::shared_ptr<VarNode>>>& varMatrix) {
  std::vector<std::shared_ptr<VarNode>> flatVarMatrix;
  flatVarMatrix.reserve(varMatrix.size() * varMatrix.front().size());
  for (const auto& varVector : varMatrix) {
    flatVarMatrix.insert(flatVarMatrix.end(), varVector.begin(),
                         varVector.end());
  }
  return flatVarMatrix;
}

ArrayVarElement2dNode::ArrayVarElement2dNode(
    InvariantGraph& graph, VarNode& rowIdx, VarNode& colIdx,
    std::vector<std::shared_ptr<VarNode>>&& flatVarMatrix, VarNode& output,
    const size_t numRows, const Int rowOffset, const Int colOffset)
    : InvariantNode(graph, {output.ptr()}, {rowIdx.ptr(), colIdx.ptr()},
                    std::move(flatVarMatrix)),
      _numRows(numRows),
      _rowOffset(rowOffset),
      _colOffset(colOffset) {}

ArrayVarElement2dNode::ArrayVarElement2dNode(
    InvariantGraph& graph, VarNode& rowIdx, VarNode& colIdx,
    std::vector<std::vector<std::shared_ptr<VarNode>>>&& varMatrix,
    VarNode& output, const Int rowOffset, const Int colOffset)
    : ArrayVarElement2dNode(graph, rowIdx, colIdx, flatten(varMatrix), output,
                            varMatrix.size(), rowOffset, colOffset) {}

void ArrayVarElement2dNode::init() {
  InvariantNode::init();
  assert(std::ranges::all_of(staticInputVarNodes(),
                             [&](const std::shared_ptr<VarNode>& vNode) {
                               return vNode->isIntVar();
                             }));
  assert(std::ranges::all_of(dynamicInputVarNodes(),
                             [&](const std::shared_ptr<VarNode>& vNode) {
                               return outputVarNode(0).isIntVar() ==
                                      vNode->isIntVar();
                             }));
}

size_t ArrayVarElement2dNode::index(const Int row, const Int col,
                                    const bool useOffset) const {
  if (useOffset) {
    assert(row >= _rowOffset && col >= _colOffset);
    const auto r = static_cast<size_t>(row - _rowOffset);
    assert(r < _numRows);
    const auto c = static_cast<size_t>(col - _colOffset);
    assert(c < numCols());
    return r * numCols() + c;
  }
  assert(0 <= row);
  assert(row < static_cast<Int>(_numRows));
  assert(0 <= col);
  assert(col < static_cast<Int>(numCols()));
  return row * numCols() + col;
}

VarNode& ArrayVarElement2dNode::at(const Int row, const Int col,
                                   const bool useOffset) const {
  return dynamicInputVarNode(index(row, col, useOffset));
}

void ArrayVarElement2dNode::postConstraint() {
  std::vector<std::vector<ConstraintVarId>> matrix(_numRows);
  for (size_t r = 0; r < _numRows; ++r) {
    matrix[r].reserve(numCols());
    for (size_t c = 0; c < numCols(); ++c) {
      matrix[r].emplace_back(
          at(static_cast<Int>(r), static_cast<Int>(c), false).constraintVarId());
    }
  }
  if (outputVarNode(0).isIntVar()) {
    constraintSolver().array_var_int_element2d(
        rowIdx().constraintVarId(),
        colIdx().constraintVarId(), matrix,
        outputVarNode(0).constraintVarId(), _rowOffset, _colOffset);
  } else {
    constraintSolver().array_var_bool_element2d(
        rowIdx().constraintVarId(),
        colIdx().constraintVarId(), matrix,
        outputVarNode(0).constraintVarId(), _rowOffset, _colOffset);
  }
}

void ArrayVarElement2dNode::updateState() {
  InvariantNode::updateState();

  const Int rowLb = rowIdx().lowerBound();
  const Int rowUb = rowIdx().upperBound();
  const Int colLb = colIdx().lowerBound();
  const Int colUb = colIdx().upperBound();

  std::vector<size_t> indicesToRemove;
  indicesToRemove.reserve(static_cast<Int>(_numRows * numCols()) -
                          (rowUb - rowLb + 1) * (colUb - colLb + 1));

  // Find invalid start rows:
  for (Int r = 0; r < rowLb - _rowOffset; ++r) {
    for (Int c = 0; c < static_cast<Int>(numCols()); ++c) {
      indicesToRemove.emplace_back(index(r, c, false));
    }
  }

  // Find invalid end rows:
  for (Int r = rowUb - _rowOffset + 1; r < static_cast<Int>(_numRows); ++r) {
    for (Int c = 0; c < static_cast<Int>(numCols()); ++c) {
      indicesToRemove.emplace_back(index(r, c, false));
    }
  }

  // Find invalid start columns:
  for (Int c = 0; c < colLb - _colOffset; ++c) {
    for (Int r = 0; r < static_cast<Int>(_numRows); ++r) {
      indicesToRemove.emplace_back(index(r, c, false));
    }
  }

  // Find invalid end columns:
  for (Int c = colUb - _colOffset + 1; c < static_cast<Int>(numCols()); ++c) {
    for (Int r = 0; r < static_cast<Int>(_numRows); ++r) {
      indicesToRemove.emplace_back(index(r, c, false));
    }
  }

  std::ranges::sort(indicesToRemove);
  const auto [first, last] = std::ranges::unique(indicesToRemove);
  indicesToRemove.erase(first, last);

  for (Int i = static_cast<Int>(indicesToRemove.size()) - 1; i >= 0; --i) {
    removeDynamicInputAtIndex(indicesToRemove[i]);
  }

  assert(!dynamicInputVarNodes().empty());

  _numRows = rowUb - rowLb + 1;
  _rowOffset = rowLb;
  _colOffset = colLb;

  if (rowIdx().constDomain()->isInterval() &&
      colIdx().constDomain()->isInterval()) {
    return;
  }

  std::vector<bool> indexIsSupported(dynamicInputVarNodes().size(), false);

  for (auto rowIter = rowIdx().constDomain()->begin();
       rowIter != rowIdx().constDomain()->end(); ++rowIter) {
    for (auto colIter = rowIdx().constDomain()->begin();
         colIter != rowIdx().constDomain()->end(); ++colIter) {
      indexIsSupported[index(*rowIter, *colIter, true)] = true;
    }
  }
  std::shared_ptr<VarNode> prevVarNode{nullptr};
  for (auto& vNode : dynamicInputVarNodes()) {
    if (vNode != nullptr) {
      prevVarNode = vNode;
      break;
    }
  }
  assert(prevVarNode != nullptr);
  for (size_t i = 1; i < dynamicInputVarNodes().size(); ++i) {
    if (indexIsSupported[i]) {
      prevVarNode = dynamicInputVarNodes()[i];
      continue;
    }
    for (size_t j = i + 1; j < dynamicInputVarNodes().size(); ++j) {
      if (!indexIsSupported[j] &&
          dynamicInputVarNodes()[i] == dynamicInputVarNodes()[j]) {
        indexIsSupported[j] = true;
      }
    }
    replaceDynamicInputVarNode(dynamicInputVarNode(i), prevVarNode);
  }
}

bool ArrayVarElement2dNode::constrainsOutput(const VarNode&) const {
  std::vector<Int> values;
  values.reserve(outputVarNode(0).constDomain()->size());
  for (auto rowIter = rowIdx().constDomain()->begin();
       rowIter != rowIdx().constDomain()->end(); ++rowIter) {
    const Int r = *rowIter - _rowOffset;
    if (r < 0) {
      continue;
      ;
    }
    if (static_cast<Int>(_numRows) < r) {
      break;
    }
    for (auto colIter = colIdx().constDomain()->begin();
         colIter != colIdx().constDomain()->end(); ++colIter) {
      const Int c = *colIter - _colOffset;
      if (c < 0) {
        continue;
        ;
      }
      if (static_cast<Int>(numCols()) < c) {
        break;
      }
      for (auto valIter = at(r, c, false).constDomain()->begin();
           valIter != at(r, c, false).constDomain()->end();
           ++valIter) {
        values.emplace_back(*valIter);
      }
    }
  }
  const SortedUniqueVector sortedVals(std::move(values));
  return !outputVarNode(0).constDomain()->contains(sortedVals);
}

void ArrayVarElement2dNode::registerOutputVars(propagation::SolverBase& solver,
                                               SolverMapping& mapping) const {
  makeSolverVar(outputVarNode(0), solver, mapping);
  assert(std::ranges::all_of(outputVarNodes(),
                             [&](const std::shared_ptr<VarNode>& vNode) {
                               return mapping.solverId(vNode) !=
                                      propagation::NULL_ID;
                             }));
}

bool ArrayVarElement2dNode::canBeReplaced() const {
  if (state() != InvariantNodeState::ACTIVE) {
    return false;
  }
  if (rowIdx().isFixed() || colIdx().isFixed()) {
    return true;
  }
  const bool allFixed = std::ranges::all_of(
      dynamicInputVarNodes(), [&](const std::shared_ptr<VarNode>& vNode) {
        return vNode->isFixed();
      });
  if (allFixed) {
    return true;
  }
  const bool allSameVar = std::ranges::all_of(
      dynamicInputVarNodes(), [&](const std::shared_ptr<VarNode>& vId) {
        return vId == dynamicInputVarNodes().front();
      });
  if (allSameVar) {
    return true;
  }
  return false;
}

bool ArrayVarElement2dNode::replace() {
  if (!canBeReplaced()) {
    return false;
  }
  const bool rowNodeIsFixed = rowIdx().isFixed();
  const bool colNodeIsFixed = colIdx().isFixed();
  if (rowNodeIsFixed && colNodeIsFixed) {
    const size_t i = index(rowIdx().lowerBound(),
                           colIdx().lowerBound(), true);
    invariantGraph().replaceVarNode(outputVarNode(0),
                                    dynamicInputVarNode(i));
    return true;
  }
  const bool allSameVar = std::ranges::all_of(
      dynamicInputVarNodes(), [&](const std::shared_ptr<VarNode>& vId) {
        return vId == dynamicInputVarNodes().front();
      });
  if (allSameVar) {
    const size_t i = index(rowIdx().lowerBound(),
                           colIdx().lowerBound(), true);
    invariantGraph().replaceVarNode(outputVarNode(0),
                                    dynamicInputVarNode(i));
    for (const auto& vNode : std::array{rowIdx().ptr(), colIdx().ptr()}) {
      if (!vNode->isFixed()) {
        vNode->tightenDomainType(
            vNode->constDomain()->isInterval()
                ? DomainType::DOM_RANGE
                : DomainType::DOM_DOMAIN);
      }
    }
    return true;
  }
  const bool allFixed = std::ranges::all_of(
      dynamicInputVarNodes(), [&](const std::shared_ptr<VarNode>& vNode) {
        return vNode->isFixed();
      });
  if (rowNodeIsFixed) {
    if (allFixed) {
      std::vector<Int> colPars;
      assert(dynamicInputVarNodes().size() % _numRows == 0);
      colPars.reserve(numCols());
      for (size_t c = 0; c < numCols(); ++c) {
        colPars.emplace_back(
            at(0, static_cast<Int>(c), false).lowerBound());
      }
      invariantGraph().addInvariantNode(std::make_shared<ArrayElementNode>(
          invariantGraph(), std::move(colPars), colIdx(),
          outputVarNode(0), _colOffset));
      return true;
    }
    std::vector<std::shared_ptr<VarNode>> colVars;
    assert(dynamicInputVarNodes().size() % _numRows == 0);
    colVars.reserve(numCols());
    for (size_t c = 0; c < numCols(); ++c) {
      colVars.emplace_back(at(0, static_cast<Int>(c), false).ptr());
    }
    invariantGraph().addInvariantNode(std::make_shared<ArrayVarElementNode>(
        invariantGraph(), colIdx(), std::move(colVars),
        outputVarNode(0), _colOffset));
    return true;
  }
  if (colNodeIsFixed) {
    if (allFixed) {
      std::vector<Int> rowPars;
      rowPars.reserve(_numRows);
      for (size_t r = 0; r < _numRows; ++r) {
        rowPars.emplace_back(at(static_cast<Int>(r), 0, false).lowerBound());
      }
      invariantGraph().addInvariantNode(std::make_shared<ArrayElementNode>(
          invariantGraph(), std::move(rowPars), rowIdx(),
          outputVarNode(0), _rowOffset));
      return true;
    }
    std::vector<std::shared_ptr<VarNode>> rowVars;
    rowVars.reserve(_numRows);
    for (size_t r = 0; r < _numRows; ++r) {
      rowVars.emplace_back(at(static_cast<Int>(r), 0, false).ptr());
    }
    invariantGraph().addInvariantNode(std::make_shared<ArrayVarElementNode>(
        invariantGraph(), rowIdx(), std::move(rowVars),
        outputVarNode(0), _rowOffset));
    return true;
  }
  assert(allFixed);
  std::vector<std::vector<Int>> parMatrix(_numRows,
                                          std::vector<Int>(numCols()));
  for (size_t r = 0; r < _numRows; ++r) {
    for (size_t c = 0; c < numCols(); ++c) {
      parMatrix[r][c] =
          at(static_cast<Int>(r), static_cast<Int>(c), false)
              .lowerBound();
    }
  }
  invariantGraph().addInvariantNode(std::make_shared<ArrayElement2dNode>(
      invariantGraph(), rowIdx(), colIdx(), std::move(parMatrix),
      outputVarNode(0), _rowOffset, _colOffset,
      outputVarNode(0).isIntVar()));
  return true;
}

void ArrayVarElement2dNode::registerNode(propagation::SolverBase& solver,
                                         SolverMapping& mapping) const {
  std::vector<std::vector<propagation::VarViewId>> varMatrix(
      _numRows, std::vector<propagation::VarViewId>{});
  for (size_t r = 0; r < _numRows; ++r) {
    varMatrix.at(r).reserve(numCols());
    for (size_t c = 0; c < numCols(); ++c) {
      varMatrix.at(r).emplace_back(mapping.solverId(at(
          static_cast<Int>(r) + _rowOffset, static_cast<Int>(c) + _colOffset)));
    }
  }

  assert(mapping.solverId(outputVarNode(0)) != propagation::NULL_ID);
  assert(mapping.solverId(outputVarNode(0)).isVar());
  solver.makeInvariant<propagation::Element2dVar>(
      solver, mapping.solverId(outputVarNode(0)),
      mapping.solverId(rowIdx()), mapping.solverId(colIdx()),
      std::move(varMatrix), _rowOffset, _colOffset);
}

std::string ArrayVarElement2dNode::dotLangIdentifier() const {
  return "var_element2d";
}

}  // namespace atlantis::invariantgraph
