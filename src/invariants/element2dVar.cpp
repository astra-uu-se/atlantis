#include "invariants/element2dVar.hpp"

static inline Int numCols(const std::vector<std::vector<VarId>>& varMatrix) {
#ifndef NDEBUG
  for (const auto& col : varMatrix) {
    assert(col.size() == varMatrix.front().size());
  }
#endif
  return varMatrix.empty() ? 0 : varMatrix.front().size();
}

Element2dVar::Element2dVar(VarId output, VarId index1, VarId index2,
                           std::vector<std::vector<VarId>> varMatrix,
                           Int offset1, Int offset2)
    : Invariant(),
      _varMatrix(varMatrix),
      _indices{index1, index2},
      _dimensions{static_cast<Int>(_varMatrix.size()), numCols(_varMatrix)},
      _offsets{offset1, offset2},
      _output(output) {
  _modifiedVars.reserve(1);
}

void Element2dVar::registerVars(Engine& engine) {
  assert(_id != NULL_ID);
  engine.registerInvariantInput(_id, _indices[0], LocalId(0));
  engine.registerInvariantInput(_id, _indices[1], LocalId(0));
  for (const auto& varRow : _varMatrix) {
    for (const VarId input : varRow) {
      engine.registerInvariantInput(_id, input, LocalId(0));
    }
  }
  registerDefinedVariable(engine, _output);
}

void Element2dVar::updateBounds(Engine& engine, bool widenOnly) {
  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::min();

  std::array<Int, 2> iLb;
  std::array<Int, 2> iUb;
  for (size_t i = 0; i < 2; ++i) {
    iLb[i] = std::max<Int>(_offsets[i], engine.lowerBound(_indices[i]));
    iUb[i] = std::min<Int>(_dimensions[i] - 1 + _offsets[i],
                           engine.upperBound(_indices[i]));
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
          lb, engine.lowerBound(_varMatrix[safeIndex1(i1)][safeIndex2(i2)]));
      ub = std::max(
          ub, engine.upperBound(_varMatrix[safeIndex1(i1)][safeIndex2(i2)]));
    }
  }
  engine.updateBounds(_output, lb, ub, widenOnly);
}

void Element2dVar::recompute(Timestamp ts, Engine& engine) {
  assert(safeIndex1(engine.value(ts, _indices[0])) <
         static_cast<size_t>(_dimensions[0]));
  assert(safeIndex2(engine.value(ts, _indices[1])) <
         static_cast<size_t>(_dimensions[1]));
  updateValue(
      ts, engine, _output,
      engine.value(ts, _varMatrix[safeIndex1(engine.value(ts, _indices[0]))]
                                 [safeIndex2(engine.value(ts, _indices[1]))]));
}

void Element2dVar::notifyInputChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId Element2dVar::nextInput(Timestamp ts, Engine& engine) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _indices[0];
    case 1:
      return _indices[1];
    case 2: {
      assert(safeIndex1(engine.value(ts, _indices[0])) <
             static_cast<size_t>(_dimensions[0]));
      assert(safeIndex2(engine.value(ts, _indices[1])) <
             static_cast<size_t>(_dimensions[1]));
      return _varMatrix[safeIndex1(engine.value(ts, _indices[0]))]
                       [safeIndex2(engine.value(ts, _indices[1]))];
    }
    default:
      return NULL_ID;  // Done
  }
}

void Element2dVar::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void Element2dVar::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}
