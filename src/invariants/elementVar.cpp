#include "invariants/elementVar.hpp"

ElementVar::ElementVar(VarId output, VarId index, std::vector<VarId> varArray,
                       Int offset)
    : Invariant(),
      _output(output),
      _index(index),
      _varArray(varArray),
      _offset(offset) {
  _modifiedVars.reserve(1);
}

void ElementVar::registerVars(Engine& engine) {
  assert(_id != NULL_ID);
  engine.registerInvariantInput(_id, _index, LocalId(0));
  for (const VarId input : _varArray) {
    engine.registerInvariantInput(_id, input, LocalId(0), true);
  }
  registerDefinedVariable(engine, _output);
}

void ElementVar::updateBounds(Engine& engine, bool widenOnly) {
  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::min();
  Int iLb = std::max<Int>(_offset, engine.lowerBound(_index));
  Int iUb = std::min<Int>(static_cast<Int>(_varArray.size()) - 1 + _offset,
                          engine.upperBound(_index));
  if (iLb > iUb) {
    iLb = _offset;
    iUb = static_cast<Int>(_varArray.size()) - 1 + _offset;
  }
  for (Int i = iLb; i <= iUb; ++i) {
    assert(_offset <= i);
    assert(i - _offset < static_cast<Int>(_varArray.size()));
    lb = std::min(lb, engine.lowerBound(_varArray[safeIndex(i)]));
    ub = std::max(ub, engine.upperBound(_varArray[safeIndex(i)]));
  }
  engine.updateBounds(_output, lb, ub, widenOnly);
}

void ElementVar::recompute(Timestamp ts, Engine& engine) {
  assert(safeIndex(engine.value(ts, _index)) < _varArray.size());
  updateValue(ts, engine, _output,
              engine.value(
                  ts, _dynamicInputVar.set(
                          ts, _varArray[safeIndex(engine.value(ts, _index))])));
}

void ElementVar::notifyInputChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId ElementVar::nextInput(Timestamp ts, Engine& engine) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _index;
    case 1: {
      assert(safeIndex(engine.value(ts, _index)) < _varArray.size());
      return _varArray[safeIndex(engine.value(ts, _index))];
    }
    default:
      return NULL_ID;  // Done
  }
}

void ElementVar::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void ElementVar::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
  _dynamicInputVar.commitIf(ts);
}
