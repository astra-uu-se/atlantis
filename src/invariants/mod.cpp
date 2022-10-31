#include "invariants/mod.hpp"

#include "core/engine.hpp"

static inline Int mod(Int xVal, Int yVal) { return xVal % std::abs(yVal); }

Mod::Mod(Engine& engine, VarId output, VarId x, VarId y)
    : Invariant(engine), _output(output), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void Mod::registerVars() {
  assert(!_id.equals(NULL_ID));
  _engine.registerInvariantInput(_id, _x, 0);
  _engine.registerInvariantInput(_id, _y, 0);
  registerDefinedVariable(_output);
}

void Mod::updateBounds(bool widenOnly) {
  _engine.updateBounds(_output, std::min(Int(0), _engine.lowerBound(_x)),
                       std::max(Int(0), _engine.upperBound(_x)), widenOnly);
}

void Mod::close(Timestamp) {
  assert(_engine.lowerBound(_y) != 0 || _engine.upperBound(_y) != 0);
  if (_engine.lowerBound(_y) <= 0 && 0 <= _engine.upperBound(_y)) {
    _zeroReplacement = _engine.upperBound(_y) >= 1 ? 1 : -1;
  }
}

void Mod::recompute(Timestamp ts) {
  assert(_zeroReplacement != 0);
  const Int denominator = _engine.value(ts, _y);
  updateValue(ts, _output,
              _engine.value(ts, _x) %
                  std::abs(denominator != 0 ? denominator : _zeroReplacement));
}

void Mod::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }

VarId Mod::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void Mod::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }

void Mod::commit(Timestamp ts) { Invariant::commit(ts); }
