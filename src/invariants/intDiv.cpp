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

void IntDiv::updateBounds(Engine& engine) {
  const Int aLb = engine.lowerBound(_a);
  const Int aUb = engine.upperBound(_a);
  const Int bLb = engine.lowerBound(_b);
  const Int bUb = engine.upperBound(_b);

  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::min();

  if (bLb <= 0 && 0 <= bUb) {
    if (bLb < 0) {
      lb = std::min(lb, aUb / -1);
      ub = std::max(ub, aLb / -1);
    }
    if (0 < bUb) {
      lb = std::min(lb, aLb / 1);
      ub = std::max(ub, aUb / 1);
    }
  }
  if (bLb != 0) {
    lb = std::min(lb, aLb / bLb);
    ub = std::max(ub, aLb / bLb);
  }
  if (bUb != 0) {
    lb = std::min(lb, aLb / bUb);
    ub = std::max(ub, aLb / bUb);
  }

  engine.updateBounds(_y, lb, ub);
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
