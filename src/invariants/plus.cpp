#include "invariants/plus.hpp"

#include "core/engine.hpp"

Plus::Plus(VarId output, VarId x, VarId y)
    : Invariant(), _output(output), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void Plus::registerVars(Engine& engine) {
  assert(!_id.equals(NULL_ID));
  engine.registerInvariantInput(_id, _x, 0);
  engine.registerInvariantInput(_id, _y, 0);
  registerDefinedVariable(engine, _output);
}

void Plus::updateBounds(Engine& engine, bool widenOnly) {
  engine.updateBounds(_output, engine.lowerBound(_x) + engine.lowerBound(_y),
                      engine.upperBound(_x) + engine.upperBound(_y), widenOnly);
}

void Plus::recompute(Timestamp ts, Engine& engine) {
  updateValue(ts, engine, _output, engine.value(ts, _x) + engine.value(ts, _y));
}

VarId Plus::nextInput(Timestamp ts, Engine&) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void Plus::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void Plus::notifyInputChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

void Plus::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}
