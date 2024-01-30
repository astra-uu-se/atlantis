#include "propagation/invariants/element2dConst.hpp"

namespace atlantis::propagation {

static inline Int numCols(const std::vector<std::vector<Int>>& matrix) {
  assert(std::all_of(matrix.begin(), matrix.end(), [&](const auto& col) {
    return col.size() == matrix.front().size();
  }));
  return matrix.empty() ? 0 : matrix.front().size();
}

Element2dConst::Element2dConst(SolverBase& solver, VarId output, VarId index1,
                               VarId index2,
                               std::vector<std::vector<Int>>&& matrix,
                               Int offset1, Int offset2)
    : Invariant(solver),
      _matrix(std::move(matrix)),
      _indices{index1, index2},
      _dimensions{static_cast<Int>(_matrix.size()), numCols(_matrix)},
      _offsets{offset1, offset2},
      _output(output) {
  _modifiedVars.reserve(1);
}

void Element2dConst::registerVars() {
  assert(_id != NULL_ID);
  _solver.registerInvariantInput(_id, _indices[0], LocalId(0), false);
  _solver.registerInvariantInput(_id, _indices[1], LocalId(0), false);
  registerDefinedVar(_output);
}

void Element2dConst::updateBounds(bool widenOnly) {
  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::min();

  std::array<Int, 2> iLb{0,0};
  std::array<Int, 2> iUb{0,0};
  for (size_t i = 0; i < 2; ++i) {
    iLb[i] = std::max<Int>(_offsets[i], _solver.lowerBound(_indices[i]));
    iUb[i] = std::min<Int>(_dimensions[i] - 1 + _offsets[i],
                           _solver.upperBound(_indices[i]));
    if (iLb[i] > iUb[i]) {
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
      lb = std::min(lb, _matrix[safeIndex1(i1)][safeIndex2(i2)]);
      ub = std::max(ub, _matrix[safeIndex1(i1)][safeIndex2(i2)]);
    }
  }
  _solver.updateBounds(_output, lb, ub, widenOnly);
}

void Element2dConst::recompute(Timestamp ts) {
  assert(safeIndex1(_solver.value(ts, _indices[0])) <
         static_cast<size_t>(_dimensions[0]));
  assert(safeIndex2(_solver.value(ts, _indices[1])) <
         static_cast<size_t>(_dimensions[1]));
  updateValue(ts, _output,
              _matrix[safeIndex1(_solver.value(ts, _indices[0]))]
                     [safeIndex2(_solver.value(ts, _indices[1]))]);
}

void Element2dConst::notifyInputChanged(Timestamp ts, LocalId) {
  recompute(ts);
}

VarId Element2dConst::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _indices[0];
    case 1:
      return _indices[1];
    default:
      return NULL_ID;  // Done
  }
}

void Element2dConst::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }
}  // namespace atlantis::propagation