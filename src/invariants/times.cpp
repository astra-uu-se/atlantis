#include "invariants/times.hpp"

#include "core/engine.hpp"

Times::Times(VarId output, VarId x, VarId y)
    : Invariant(), _output(output), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void Times::registerVars(Engine& engine) {
  assert(!_id.equals(NULL_ID));
  engine.registerInvariantInput(_id, _x, 0);
  engine.registerInvariantInput(_id, _y, 0);
  registerDefinedVariable(engine, _output);
}

void Times::updateBounds(Engine& engine, bool widenOnly) {
  const Int xLb = engine.lowerBound(_x);
  const Int xUb = engine.upperBound(_x);
  const Int yLb = engine.lowerBound(_y);
  const Int yUb = engine.upperBound(_y);
  const std::array<const Int, 4> vals{xLb * yLb, xLb * yUb, xUb * yLb,
                                      xUb * yUb};
  const auto [lb, ub] = std::minmax_element(vals.begin(), vals.end());
  engine.updateBounds(_output, *lb, *ub, widenOnly);
}

void Times::recompute(Timestamp ts, Engine& engine) {
  updateValue(ts, engine, _output, engine.value(ts, _x) * engine.value(ts, _y));
}

VarId Times::nextInput(Timestamp ts, Engine&) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void Times::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void Times::notifyInputChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

void Times::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}
