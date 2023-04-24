#include "invariants/plus.hpp"

Plus::Plus(Engine& engine, VarId output, VarId x, VarId y)
    : Invariant(engine), _output(output), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void Plus::registerVars() {
  assert(!_id.equals(NULL_ID));
  _engine.registerInvariantInput(_id, _x, 0);
  _engine.registerInvariantInput(_id, _y, 0);
  registerDefinedVariable(_output);
}

void Plus::updateBounds(bool widenOnly) {
  _engine.updateBounds(_output, _engine.lowerBound(_x) + _engine.lowerBound(_y),
                       _engine.upperBound(_x) + _engine.upperBound(_y),
                       widenOnly);
}

void Plus::recompute(Timestamp ts) {
  updateValue(ts, _output, _engine.value(ts, _x) + _engine.value(ts, _y));
}

VarId Plus::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void Plus::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }

void Plus::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }
