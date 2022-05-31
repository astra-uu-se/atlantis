#include "invariants/intDiv.hpp"

#include "core/engine.hpp"

IntDiv::IntDiv(VarId output, VarId x, VarId y)
    : Invariant(), _output(output), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void IntDiv::registerVars(Engine& engine) {
  assert(!_id.equals(NULL_ID));
  engine.registerInvariantInput(_id, _x, 0);
  engine.registerInvariantInput(_id, _y, 0);
  registerDefinedVariable(engine, _output);
}

void IntDiv::updateBounds(Engine& engine, bool widenOnly) {
  const Int xLb = engine.lowerBound(_x);
  const Int xUb = engine.upperBound(_x);
  const Int yLb = engine.lowerBound(_y);
  const Int yUb = engine.upperBound(_y);

  assert(yLb != 0 || yUb != 0);

  std::vector<Int> denominators;
  denominators.reserve(4);

  if (yLb <= 0 && 0 < yUb) {
    denominators.emplace_back(1);
  }
  if (yLb < 0 && 0 <= yUb) {
    denominators.emplace_back(-1);
  }
  if (yLb != 0) {
    denominators.emplace_back(yLb);
  }
  if (yUb != 0) {
    denominators.emplace_back(yUb);
  }

  assert(denominators.size() > 0);
  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::min();
  for (const Int d : denominators) {
    lb = std::min(lb, std::min(xLb / d, xUb / d));
    ub = std::max(ub, std::max(xLb / d, xUb / d));
  }

  engine.updateBounds(_output, lb, ub, widenOnly);
}

void IntDiv::close(Timestamp, Engine& engine) {
  assert(!_id.equals(NULL_ID));
  engine.registerInvariantInput(_id, _x, 0);
  engine.registerInvariantInput(_id, _y, 0);

  const Int lb = engine.lowerBound(_y);
  const Int ub = engine.upperBound(_y);

  assert(lb != 0 || ub != 0);
  if (lb <= 0 && 0 <= ub) {
    _zeroReplacement = ub >= 1 ? 1 : -1;
  }
}

void IntDiv::recompute(Timestamp ts, Engine& engine) {
  assert(_zeroReplacement != 0);
  const Int denominator = engine.value(ts, _y);
  updateValue(ts, engine, _output,
              engine.value(ts, _x) /
                  (denominator != 0 ? denominator : _zeroReplacement));
}

VarId IntDiv::nextInput(Timestamp ts, Engine&) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void IntDiv::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void IntDiv::notifyInputChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

void IntDiv::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}
