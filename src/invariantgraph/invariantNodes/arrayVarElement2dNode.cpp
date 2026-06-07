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
    InvariantGraph& graph, const VarNodeId rowIdx, const VarNodeId colIdx,
    std::vector<VarNodeId>&& flatVarMatrix, const VarNodeId output, const size_t numRows,
    const Int rowOffset, const Int colOffset)
    : InvariantNode(graph, {output}, {rowIdx, colIdx},
                    std::move(flatVarMatrix)),
      _numRows(numRows),
      _rowOffset(rowOffset),
      _colOffset(colOffset) {}

ArrayVarElement2dNode::ArrayVarElement2dNode(
    InvariantGraph& graph, const VarNodeId rowIdx, const VarNodeId colIdx,
    std::vector<std::vector<VarNodeId>>&& varMatrix, const VarNodeId output,
    const Int rowOffset, const Int colOffset)
    : ArrayVarElement2dNode(graph, rowIdx, colIdx, flatten(varMatrix), output,
                            varMatrix.size(), rowOffset, colOffset) {}

void ArrayVarElement2dNode::init(const InvariantNodeId id) {
  InvariantNode::init(id);
  assert(std::ranges::all_of(
      staticInputVarNodeIds(),
      [&](const VarNodeId node) {
        return varNodeConst(node).isIntVar();
      }));
  assert(std::ranges::all_of(
      dynamicInputVarNodeIds(),
      [&](const VarNodeId node) {
        return outputVarNodeConst(0).isIntVar() ==
               varNodeConst(node).isIntVar();
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

VarNodeId ArrayVarElement2dNode::at(const Int row, const Int col,
                                    const bool useOffset) const {
  return dynamicInputVarNodeIds()[index(row, col, useOffset)];
}

void ArrayVarElement2dNode::postConstraint() {
  std::vector<std::vector<ConstraintVarId>> matrix(_numRows);
  for (size_t r = 0; r < _numRows; ++r) {
    matrix[r].reserve(numCols());
    for (size_t c = 0; c < numCols(); ++c) {
      matrix[r].emplace_back(
          varNodeConst(at(static_cast<Int>(r), static_cast<Int>(c), false))
              .constraintVarId());
    }
  }
  if (outputVarNodeConst(0).isIntVar()) {
    constraintSolver().array_var_int_element2d(
        varNodeConst(rowIdx()).constraintVarId(),
        varNodeConst(colIdx()).constraintVarId(), matrix,
        outputVarNodeConst(0).constraintVarId(), _rowOffset, _colOffset);
  } else {
    constraintSolver().array_var_bool_element2d(
        varNodeConst(rowIdx()).constraintVarId(),
        varNodeConst(colIdx()).constraintVarId(), matrix,
        outputVarNodeConst(0).constraintVarId(), _rowOffset, _colOffset);
  }
}

void ArrayVarElement2dNode::updateState() {
  InvariantNode::updateState();

  const Int rowLb = varNodeConst(rowIdx()).lowerBound();
  const Int colLb = varNodeConst(colIdx()).lowerBound();

  std::vector<size_t> indicesToRemove;
  indicesToRemove.reserve((rowLb - _rowOffset) * (colLb - _colOffset));

  // Find invalid start rows:
  for (Int r = 0; r < rowLb - _rowOffset; ++r) {
    for (Int c = 0; c < static_cast<Int>(numCols()); ++c) {
      indicesToRemove.emplace_back(index(r, c, false));
    }
  }

  // Find invalid end rows:
  for (Int r = varNodeConst(rowIdx()).upperBound() - _rowOffset + 1; r < static_cast<Int>(_numRows); ++r) {
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
  for (Int c = varNodeConst(colIdx()).upperBound() - _colOffset + 1; c < static_cast<Int>(numCols()); ++c) {
    for (Int r = 0; r < static_cast<Int>(_numRows); ++r) {
      indicesToRemove.emplace_back(index(r, c, false));
    }
  }
  const Int rowSize = varNodeConst(rowIdx()).upperBound() -
                      varNodeConst(rowIdx()).lowerBound() + 1;
  assert(rowSize <= static_cast<Int>(_numRows));

  const Int colSize = varNodeConst(colIdx()).upperBound() -
                      varNodeConst(colIdx()).lowerBound() + 1;
  assert(colSize <= static_cast<Int>(numCols()));


  std::ranges::sort(indicesToRemove);
  const auto [first, last] = std::ranges::unique(indicesToRemove);
  indicesToRemove.erase(first, last);

  for (Int i = static_cast<Int>(indicesToRemove.size()) - 1; i >= 0; --i) {
    removeDynamicInputAtIndex(indicesToRemove[i]);
  }

  assert(!dynamicInputVarNodeIds().empty());

  _numRows = rowSize;
  _rowOffset = rowLb;
  _colOffset = colLb;

  if (varNodeConst(rowIdx()).constDomain()->isInterval() &&
      varNodeConst(colIdx()).constDomain()->isInterval()) {
    return;
  }

  std::vector<bool> indexIsSupported(dynamicInputVarNodeIds().size(), false);

  for (auto rowIter = varNodeConst(rowIdx()).constDomain()->begin();
       rowIter != varNodeConst(rowIdx()).constDomain()->end(); ++rowIter) {
    for (auto colIter = varNodeConst(rowIdx()).constDomain()->begin();
         colIter != varNodeConst(rowIdx()).constDomain()->end(); ++colIter) {
      indexIsSupported[index(*rowIter, *colIter, true)] = true;
    }
  }
  VarNodeId prevVarNodeId{NULL_NODE_ID};
  for (const VarNodeId vId : dynamicInputVarNodeIds()) {
    if (vId != NULL_NODE_ID) {
      prevVarNodeId = vId;
      break;
    }
  }
  assert(prevVarNodeId != NULL_NODE_ID);
  for (size_t i = 1; i < dynamicInputVarNodeIds().size(); ++i) {
    if (indexIsSupported[i]) {
      prevVarNodeId = dynamicInputVarNodeIds()[i];
      continue;
    }
    for (size_t j = i + 1; j < dynamicInputVarNodeIds().size(); ++j) {
      if (!indexIsSupported[j] &&
          dynamicInputVarNodeIds()[i] == dynamicInputVarNodeIds()[j]) {
        indexIsSupported[j] = true;
      }
    }
    replaceDynamicInputVarNode(dynamicInputVarNodeIds()[i], prevVarNodeId);
  }
}

void ArrayVarElement2dNode::registerOutputVars(propagation::SolverBase& solver,
                                               SolverMapping& mapping) const {
  makeSolverVar(outputVarNodeIds().front(), solver, mapping);
  assert(std::ranges::all_of(
      outputVarNodeIds().begin(), outputVarNodeIds().end(),
      [&](const VarNodeId vId) {
        return mapping.solverId(vId) != propagation::NULL_ID;
      }));
}

bool ArrayVarElement2dNode::canBeReplaced() const {
  if (state() != InvariantNodeState::ACTIVE) {
    return false;
  }
  if (varNodeConst(rowIdx()).isFixed() ||
      varNodeConst(colIdx()).isFixed()) {
    return true;
  }
  const bool allFixed = std::ranges::all_of(
      dynamicInputVarNodeIds(),
      [&](const VarNodeId vId) { return varNodeConst(vId).isFixed(); });
  if (allFixed) {
    return true;
  }
  const bool allSameVar =
      std::ranges::all_of(dynamicInputVarNodeIds(), [&](const VarNodeId vId) {
        return vId == dynamicInputVarNodeIds().front();
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
  const bool rowNodeIsFixed = varNodeConst(rowIdx()).isFixed();
  const bool colNodeIsFixed = varNodeConst(colIdx()).isFixed();
  if (rowNodeIsFixed && colNodeIsFixed) {
    const size_t i = index(varNodeConst(rowIdx()).lowerBound(),
                           varNodeConst(colIdx()).lowerBound(), true);
    invariantGraph().replaceVarNode(outputVarNodeIds().front(),
                                    dynamicInputVarNodeIds()[i]);
    return true;
  }
  const bool allSameVar =
      std::ranges::all_of(dynamicInputVarNodeIds(), [&](const VarNodeId vId) {
        return vId == dynamicInputVarNodeIds().front();
      });
  if (allSameVar) {
    const size_t i = index(varNodeConst(rowIdx()).lowerBound(),
                           varNodeConst(colIdx()).lowerBound(), true);
    invariantGraph().replaceVarNode(outputVarNodeIds().front(),
                                    dynamicInputVarNodeIds()[i]);
    for (const VarNodeId vId : std::array{rowIdx(), colIdx()}) {
      if (!varNodeConst(vId).isFixed()) {
        varNode(vId).tightenDomainType(
            varNodeConst(vId).constDomain()->isInterval()
                ? DomainType::DOM_RANGE
                : DomainType::DOM_DOMAIN);
      }
    }
    return true;
  }
  const bool allFixed = std::ranges::all_of(
      dynamicInputVarNodeIds(),
      [&](const VarNodeId vId) { return varNodeConst(vId).isFixed(); });
  if (rowNodeIsFixed) {
    if (allFixed) {
      std::vector<Int> colPars;
      assert(dynamicInputVarNodeIds().size() % _numRows == 0);
      colPars.reserve(numCols());
      for (size_t c = 0; c < numCols(); ++c) {
        colPars.emplace_back(
            varNodeConst(at(0, static_cast<Int>(c), false)).lowerBound());
      }
      invariantGraph().addInvariantNode(std::make_shared<ArrayElementNode>(
          invariantGraph(), std::move(colPars), colIdx(),
          outputVarNodeIds().front(), _colOffset));
      return true;
    }
    std::vector<VarNodeId> colVars;
    assert(dynamicInputVarNodeIds().size() % _numRows == 0);
    colVars.reserve(numCols());
    for (size_t c = 0; c < numCols(); ++c) {
      colVars.emplace_back(at(0, static_cast<Int>(c), false));
    }
    invariantGraph().addInvariantNode(std::make_shared<ArrayVarElementNode>(
        invariantGraph(), colIdx(), std::move(colVars),
        outputVarNodeIds().front(), _colOffset));
    return true;
  }
  if (colNodeIsFixed) {
    if (allFixed) {
      std::vector<Int> rowPars;
      rowPars.reserve(_numRows);
      for (size_t r = 0; r < _numRows; ++r) {
        rowPars.emplace_back(
            varNodeConst(at(static_cast<Int>(r), 0, false)).lowerBound());
      }
      invariantGraph().addInvariantNode(std::make_shared<ArrayElementNode>(
          invariantGraph(), std::move(rowPars), rowIdx(),
          outputVarNodeIds().front(), _rowOffset));
      return true;
    }
    std::vector<VarNodeId> rowVars;
    rowVars.reserve(_numRows);
    for (size_t r = 0; r < _numRows; ++r) {
      rowVars.emplace_back(at(static_cast<Int>(r), 0, false));
    }
    invariantGraph().addInvariantNode(std::make_shared<ArrayVarElementNode>(
        invariantGraph(), rowIdx(), std::move(rowVars),
        outputVarNodeIds().front(), _rowOffset));
    return true;
  }
  assert(false);
  return false;
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

  assert(mapping.solverId(outputVarNodeIds().front()) != propagation::NULL_ID);
  assert(mapping.solverId(outputVarNodeIds().front()).isVar());
  solver.makeInvariant<propagation::Element2dVar>(
      solver, mapping.solverId(outputVarNodeIds().front()),
      mapping.solverId(rowIdx()), mapping.solverId(colIdx()),
      std::move(varMatrix), _rowOffset, _colOffset);
}

std::string ArrayVarElement2dNode::dotLangIdentifier() const {
  return "var_element2d";
}

}  // namespace atlantis::invariantgraph
