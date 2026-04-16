#include "atlantis/invariantgraph/invariantNodes/arrayElement2dNode.hpp"

#include <algorithm>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/mpl/at.hpp>
#include <boost/xpressive/detail/core/access.hpp>

#include "../parseHelper.hpp"
#include "atlantis/invariantgraph/invariantGraph.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayElementNode.hpp"
#include "atlantis/invariantgraph/varNode.hpp"
#include "atlantis/propagation/invariants/element2dConst.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/utils/domains.hpp"

namespace atlantis::invariantgraph {

Int getValue(const std::vector<std::vector<Int>>& matrix, Int row, Int col,
             Int rowOffset, Int colOffset) {
  return matrix.at(row - rowOffset).at(col - colOffset);
}

static std::vector<std::vector<Int>> toIntMatrix(
    std::vector<std::vector<bool>>&& boolMatrix) {
  std::vector<std::vector<Int>> intMatrix;
  intMatrix.reserve(boolMatrix.size());
  for (auto& row : boolMatrix) {
    intMatrix.emplace_back();
    intMatrix.back().reserve(row.size());
    for (const bool par : row) {
      intMatrix.back().emplace_back(par ? 0 : 1);
    }
  }
  return intMatrix;
}

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
    std::vector<std::vector<bool>>&& parMatrix, const VarNodeId output,
    const Int rowOffset, const Int colOffset)
    : ArrayElement2dNode(graph, rowIdx, colIdx,
                         toIntMatrix(std::move(parMatrix)), output, rowOffset,
                         colOffset, false) {}
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
        varNodeConst(colIdx()).constraintVarId(), _parMatrix,
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
    const Int val = outputVarNodeConst(0).lowerBound();
    std::vector<bool> validRowIndices(_parMatrix.size(), false);
    std::vector<bool> validColIndices(_parMatrix.front().size(), false);

    for (size_t r = 0; r < _parMatrix.size(); ++r) {
      for (size_t c = 0; c < _parMatrix.front().size(); ++c) {
        if (_parMatrix[r][c] == val) {
          validRowIndices[r] = true;
          validColIndices[c] = true;
        }
      }
    }
    std::array<std::vector<Int>, 2> validVals;

    validVals[0].reserve(validRowIndices.size());
    for (Int i = 0; i < static_cast<Int>(validRowIndices.size()); ++i) {
      if (validRowIndices[i]) {
        validVals[0].emplace_back(i + _rowOffset);
      }
    }
    validVals[1].reserve(validColIndices.size());
    for (Int i = 0; i < static_cast<Int>(validColIndices.size()); ++i) {
      if (validColIndices[i]) {
        validVals[1].emplace_back(i + _colOffset);
      }
    }
    const std::array<VarNodeId, 2> indices{rowIdx(), colIdx()};
    for (size_t i = 0; i < indices.size(); ++i) {
      varNode(indices[i])
          .domain()
          ->removeAllValuesExcept(SortedUniqueVector(std::move(validVals[i])));
      if (varNodeConst(indices[i]).isFixed()) {
        varNode(indices[i]).setDomainType(DomainType::DOM_FIXED);
      } else if (varNode(indices[i]).domain()->isInterval()) {
        varNode(indices[i]).setDomainType(DomainType::DOM_RANGE);
      } else {
        varNode(indices[i]).setDomainType(DomainType::DOM_DOMAIN);
      }
    }

    setState(InvariantNodeState::SUBSUMED);
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
      std::vector<std::vector<Int>>(_parMatrix), _rowOffset, _colOffset);
}

std::string ArrayElement2dNode::dotLangIdentifier() const {
  return "element2d";
}

}  // namespace atlantis::invariantgraph
