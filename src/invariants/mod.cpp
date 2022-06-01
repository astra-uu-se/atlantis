#include "invariants/mod.hpp"

#include "core/engine.hpp"

static inline Int mod(Int xVal, Int yVal) { return xVal % std::abs(yVal); }

Mod::Mod(VarId output, VarId x, VarId y)
    : Invariant(), _output(output), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void Mod::registerVars(Engine& engine) {
  assert(!_id.equals(NULL_ID));
  engine.registerInvariantInput(_id, _x, 0);
  engine.registerInvariantInput(_id, _y, 0);
  registerDefinedVariable(engine, _output);
}

void Mod::updateBounds(Engine& engine, bool widenOnly) {
  engine.updateBounds(_output, std::min(Int(0), engine.lowerBound(_x)),
                      std::max(Int(0), engine.upperBound(_x)), widenOnly);
}

void Mod::close(Timestamp, Engine& engine) {
  assert(engine.lowerBound(_y) != 0 || engine.upperBound(_y) != 0);
  if (engine.lowerBound(_y) <= 0 && 0 <= engine.upperBound(_y)) {
    _zeroReplacement = engine.upperBound(_y) >= 1 ? 1 : -1;
  }
}

void Mod::recompute(Timestamp ts, Engine& engine) {
  assert(_zeroReplacement != 0);
  const Int denominator = engine.value(ts, _y);
  updateValue(ts, engine, _output,
              engine.value(ts, _x) %
                  std::abs(denominator != 0 ? denominator : _zeroReplacement));
}

void Mod::notifyInputChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId Mod::nextInput(Timestamp ts, Engine&) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void Mod::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void Mod::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}
