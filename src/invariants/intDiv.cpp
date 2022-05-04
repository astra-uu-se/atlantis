#include "invariants/intDiv.hpp"

#include "core/engine.hpp"

IntDiv::IntDiv(VarId a, VarId b, VarId y)
    : Invariant(NULL_ID), _a(a), _b(b), _y(y) {
  _modifiedVars.reserve(1);
}

void IntDiv::registerVars(Engine& engine) {
  assert(!_id.equals(NULL_ID));
  engine.registerInvariantInput(_id, _a, 0);
  engine.registerInvariantInput(_id, _b, 0);
  registerDefinedVariable(engine, _y);
}

void IntDiv::updateBounds(Engine& engine, bool widenOnly) {
  const Int aLb = engine.lowerBound(_a);
  const Int aUb = engine.upperBound(_a);
  const Int bLb = engine.lowerBound(_b);
  const Int bUb = engine.upperBound(_b);

  assert(bLb != 0 || bUb != 0);

  std::vector<Int> denominators;
  denominators.reserve(4);

  if (bLb <= 0 && 0 < bUb) {
    denominators.emplace_back(1);
  }
  if (bLb < 0 && 0 <= bUb) {
    denominators.emplace_back(-1);
  }
  if (bLb != 0) {
    denominators.emplace_back(bLb);
  }
  if (bUb != 0) {
    denominators.emplace_back(bUb);
  }

  assert(denominators.size() > 0);
  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::min();
  for (const Int d : denominators) {
    lb = std::min(lb, std::min(aLb / d, aUb / d));
    ub = std::max(ub, std::max(aLb / d, aUb / d));
  }

  engine.updateBounds(_y, lb, ub, widenOnly);
}

void IntDiv::close(Timestamp, Engine& engine) {
  assert(!_id.equals(NULL_ID));
  engine.registerInvariantInput(_id, _a, 0);
  engine.registerInvariantInput(_id, _b, 0);

  const Int lb = engine.lowerBound(_b);
  const Int ub = engine.upperBound(_b);

  assert(lb != 0 || ub != 0);
  if (lb <= 0 && 0 <= ub) {
    _zeroReplacement = ub >= 1 ? 1 : -1;
  }
}

void IntDiv::recompute(Timestamp ts, Engine& engine) {
  assert(_zeroReplacement != 0);
  const Int denominator = engine.value(ts, _b);
  updateValue(ts, engine, _y,
              engine.value(ts, _a) /
                  (denominator != 0 ? denominator : _zeroReplacement));
}

VarId IntDiv::nextInput(Timestamp ts, Engine&) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _a;
    case 1:
      return _b;
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
