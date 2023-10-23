#include "propagation/invariants/intDiv.hpp"

namespace atlantis::propagation {

IntDiv::IntDiv(Engine& engine, VarId output, VarId x, VarId y)
    : Invariant(engine), _output(output), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void IntDiv::registerVars() {
  assert(!_id.equals(NULL_ID));
  _engine.registerInvariantInput(_id, _x, 0);
  _engine.registerInvariantInput(_id, _y, 0);
  registerDefinedVariable(_output);
}

void IntDiv::updateBounds(bool widenOnly) {
  const Int xLb = _engine.lowerBound(_x);
  const Int xUb = _engine.upperBound(_x);
  const Int yLb = _engine.lowerBound(_y);
  const Int yUb = _engine.upperBound(_y);

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

  _engine.updateBounds(_output, lb, ub, widenOnly);
}

void IntDiv::close(Timestamp) {
  assert(!_id.equals(NULL_ID));
  _engine.registerInvariantInput(_id, _x, 0);
  _engine.registerInvariantInput(_id, _y, 0);

  const Int lb = _engine.lowerBound(_y);
  const Int ub = _engine.upperBound(_y);

  assert(lb != 0 || ub != 0);
  if (lb <= 0 && 0 <= ub) {
    _zeroReplacement = ub >= 1 ? 1 : -1;
  }
}

void IntDiv::recompute(Timestamp ts) {
  assert(_zeroReplacement != 0);
  const Int denominator = _engine.value(ts, _y);
  updateValue(ts, _output,
              _engine.value(ts, _x) /
                  (denominator != 0 ? denominator : _zeroReplacement));
}

VarId IntDiv::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void IntDiv::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }

void IntDiv::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }
}  // namespace atlantis::propagation