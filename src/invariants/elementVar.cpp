#include "invariants/elementVar.hpp"

#include "core/engine.hpp"

ElementVar::ElementVar(Engine& engine, VarId output, VarId index,
                       std::vector<VarId> varArray, Int offset)
    : Invariant(engine),
      _output(output),
      _index(index),
      _varArray(varArray),
      _offset(offset) {
  _modifiedVars.reserve(1);
}

void ElementVar::registerVars() {
  assert(_id != NULL_ID);
  _engine.registerInvariantInput(_id, _index, LocalId(0));
  for (const VarId input : _varArray) {
    _engine.registerInvariantInput(_id, input, LocalId(0), true);
  }
  registerDefinedVariable(_output);
}

void ElementVar::updateBounds(bool widenOnly) {
  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::min();
  Int iLb = std::max<Int>(_offset, _engine.lowerBound(_index));
  Int iUb = std::min<Int>(static_cast<Int>(_varArray.size()) - 1 + _offset,
                          _engine.upperBound(_index));
  if (iLb > iUb) {
    iLb = _offset;
    iUb = static_cast<Int>(_varArray.size()) - 1 + _offset;
  }
  for (Int i = iLb; i <= iUb; ++i) {
    assert(_offset <= i);
    assert(i - _offset < static_cast<Int>(_varArray.size()));
    lb = std::min(lb, _engine.lowerBound(_varArray[safeIndex(i)]));
    ub = std::max(ub, _engine.upperBound(_varArray[safeIndex(i)]));
  }
  _engine.updateBounds(_output, lb, ub, widenOnly);
}

void ElementVar::recompute(Timestamp ts) {
  assert(safeIndex(_engine.value(ts, _index)) < _varArray.size());
  updateValue(
      ts, _output,
      _engine.value(ts,
                    _dynamicInputVar.set(
                        ts, _varArray[safeIndex(_engine.value(ts, _index))])));
}

void ElementVar::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }

VarId ElementVar::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _index;
    case 1: {
      assert(safeIndex(_engine.value(ts, _index)) < _varArray.size());
      return _varArray[safeIndex(_engine.value(ts, _index))];
    }
    default:
      return NULL_ID;  // Done
  }
}

void ElementVar::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }

void ElementVar::commit(Timestamp ts) { Invariant::commit(ts); }
