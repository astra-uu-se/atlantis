#include "invariants/times.hpp"

#include "core/engine.hpp"

Times::Times(Engine& engine, VarId output, VarId x, VarId y)
    : Invariant(engine), _output(output), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void Times::registerVars() {
  assert(!_id.equals(NULL_ID));
  _engine.registerInvariantInput(_id, _x, 0);
  _engine.registerInvariantInput(_id, _y, 0);
  registerDefinedVariable(_output);
}

void Times::updateBounds(bool widenOnly) {
  const Int xLb = _engine.lowerBound(_x);
  const Int xUb = _engine.upperBound(_x);
  const Int yLb = _engine.lowerBound(_y);
  const Int yUb = _engine.upperBound(_y);
  const std::array<const Int, 4> vals{xLb * yLb, xLb * yUb, xUb * yLb,
                                      xUb * yUb};
  const auto [lb, ub] = std::minmax_element(vals.begin(), vals.end());
  _engine.updateBounds(_output, *lb, *ub, widenOnly);
}

void Times::recompute(Timestamp ts) {
  updateValue(ts, _output, _engine.value(ts, _x) * _engine.value(ts, _y));
}

VarId Times::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void Times::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }

void Times::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }

void Times::commit(Timestamp ts) { Invariant::commit(ts); }
