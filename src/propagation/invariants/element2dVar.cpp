#include "atlantis/propagation/invariants/element2dVar.hpp"

#include <algorithm>
#include <limits>
#include <vector>

namespace atlantis::propagation {

static inline Int numCols(
    const std::vector<std::vector<VarViewId>>& varMatrix) {
  assert(std::all_of(varMatrix.begin(), varMatrix.end(), [&](const auto& col) {
    return col.size() == varMatrix.front().size();
  }));
  return varMatrix.empty() ? 0 : static_cast<Int>(varMatrix.front().size());
}

static inline size_t size(const std::vector<std::vector<VarId>>& varMatrix) {
  return varMatrix.empty() ? 0 : (varMatrix.front().size() * varMatrix.size());
}

static inline LocalId toLocalId(
    const std::vector<std::vector<VarId>>& varMatrix, size_t index1,
    size_t index2) {
  assert(index1 < varMatrix.size());
  assert(index2 < static_cast<size_t>(numCols(varMatrix)));
  return LocalId(
      varMatrix.empty() ? 0 : (varMatrix.front().size() * index1 + index2));
}

Element2dVar::Element2dVar(SolverBase& solver, VarId output, VarViewId index1,
                           VarViewId index2,
                           std::vector<std::vector<VarViewId>>&& varMatrix,
                           Int offset1, Int offset2)
    : Invariant(solver),
      _varMatrix(std::move(varMatrix)),
      _indices{index1, index2},
      _dimensions{static_cast<Int>(_varMatrix.size()), numCols(_varMatrix)},
      _offsets{offset1, offset2},
      _output(output) {}

Element2dVar::Element2dVar(SolverBase& solver, VarViewId output,
                           VarViewId index1, VarViewId index2,
                           std::vector<std::vector<VarViewId>>&& varMatrix,
                           Int offset1, Int offset2)
    : Element2dVar(solver, VarId(output), index1, index2, std::move(varMatrix),
                   offset1, offset2) {
  assert(output.isVar());
}

void Element2dVar::registerVars() {
  assert(_id != NULL_ID);
  for (const auto& varRow : _varMatrix) {
    for (const VarViewId& input : varRow) {
      _solver.registerInvariantInput(_id, input, true);
    }
  }
  _solver.registerInvariantInput(_id, _indices[0], false);
  _solver.registerInvariantInput(_id, _indices[1], false);
  registerDefinedVar(_output);
}

void Element2dVar::updateBounds(bool widenOnly) {
  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::min();

  std::array<Int, 2> iLb{0, 0};
  std::array<Int, 2> iUb{0, 0};
  for (size_t i = 0; i < 2; ++i) {
    iLb[i] = std::max<Int>(_offsets[i], _solver.lowerBound(_indices[i]));
    iUb[i] = std::min<Int>(_dimensions[i] - 1 + _offsets[i],
                           _solver.upperBound(_indices[i]));
    if (iLb > iUb) {
      iLb[i] = _offsets[i];
      iUb[i] = _dimensions[i] - 1 + _offsets[i];
    }
  }

  for (Int i1 = iLb[0]; i1 <= iUb[0]; ++i1) {
    assert(_offsets[0] <= i1);
    assert(i1 - _offsets[0] < _dimensions[0]);
    for (Int i2 = iLb[1]; i2 <= iUb[1]; ++i2) {
      assert(_offsets[1] <= i2);
      assert(i2 - _offsets[1] < _dimensions[1]);
      lb = std::min(
          lb, _solver.lowerBound(_varMatrix[safeIndex1(i1)][safeIndex2(i2)]));
      ub = std::max(
          ub, _solver.upperBound(_varMatrix[safeIndex1(i1)][safeIndex2(i2)]));
    }
  }
  _solver.updateBounds(_output, lb, ub, widenOnly);
}

VarViewId Element2dVar::dynamicInputVar(Timestamp ts) const noexcept {
  return _varMatrix[safeIndex1(_solver.value(ts, _indices[0]))]
                   [safeIndex2(_solver.value(ts, _indices[1]))];
}

void Element2dVar::recompute(Timestamp ts) {
  const size_t index1 = safeIndex1(_solver.value(ts, _indices[0]));
  assert(index1 < static_cast<size_t>(_dimensions[0]));

  const size_t index2 = safeIndex2(_solver.value(ts, _indices[1]));
  assert(index2 < static_cast<size_t>(_dimensions[1]));

  for (size_t i = 0; i < _varMatrix.size(); ++i) {
    for (size_t j = 0; j < _varMatrix[i].size(); ++j) {
      if (i != index1 || j != index2) {
        makeDynamicInputInactive(ts, toLocalId(_varMatrix, i, j));
      }
    }
    makeDynamicInputActive(ts, toLocalId(_varMatrix, index1, index2));
  }

  updateValue(ts, _output, _solver.value(ts, _varMatrix[index1][index2]));
}

void Element2dVar::notifyInputChanged(Timestamp ts, LocalId localId) {
  const size_t index1 = safeIndex1(_solver.value(ts, _indices[0]));
  const size_t index2 = safeIndex2(_solver.value(ts, _indices[1]));
  if (localId >= size(_varMatrix)) {
    if (localId == size(_varMatrix)) {
      makeDynamicInputInactive(
          ts, toLocalId(_varMatrix, safeIndex1(_committedIndices[0]), index2));
    } else {
      makeDynamicInputInactive(
          ts, toLocalId(_varMatrix, index1, safeIndex2(_committedIndices[1])));
    }
    makeDynamicInputActive(ts, toLocalId(_varMatrix, index1, index2));
  }
  updateValue(ts, _output, _solver.value(ts, _varMatrix[index1][index2]));
}

VarViewId Element2dVar::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _indices[0];
    case 1:
      return _indices[1];
    case 2: {
      assert(safeIndex1(_solver.value(ts, _indices[0])) <
             static_cast<size_t>(_dimensions[0]));
      assert(safeIndex2(_solver.value(ts, _indices[1])) <
             static_cast<size_t>(_dimensions[1]));
      return _varMatrix[safeIndex1(_solver.value(ts, _indices[0]))]
                       [safeIndex2(_solver.value(ts, _indices[1]))];
    }
    default:
      return NULL_ID;  // Done
  }
}

void Element2dVar::notifyCurrentInputChanged(Timestamp ts) {
  assert(safeIndex1(_solver.value(ts, _indices[0])) <
         static_cast<size_t>(_dimensions[0]));
  assert(safeIndex2(_solver.value(ts, _indices[1])) <
         static_cast<size_t>(_dimensions[1]));
  updateValue(ts, _output,
              _solver.value(
                  ts, _varMatrix[safeIndex1(_solver.value(ts, _indices[0]))]
                                [safeIndex2(_solver.value(ts, _indices[1]))]));
}

void Element2dVar::commit(Timestamp ts) {
  Invariant::commit(ts);
  _committedIndices[0] = safeIndex1(_solver.committedValue(_indices[0]));
  _committedIndices[1] = safeIndex2(_solver.committedValue(_indices[1]));
}

}  // namespace atlantis::propagation
