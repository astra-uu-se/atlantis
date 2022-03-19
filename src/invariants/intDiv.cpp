#include "invariants/intDiv.hpp"

#include "core/engine.hpp"

IntDiv::IntDiv(VarId a, VarId b, VarId y)
    : Invariant(NULL_ID), _a(a), _b(b), _y(y) {
  _modifiedVars.reserve(1);
}

void IntDiv::init([[maybe_unused]] Timestamp ts, Engine& engine) {
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

void IntDiv::updateBounds(Engine& engine) {
  const Int aLb = engine.lowerBound(_a);
  const Int aUb = engine.upperBound(_a);
  const Int bLb = engine.lowerBound(_b);
  const Int bUb = engine.upperBound(_b);

  const std::array<Int, 4> vals{aLb / bLb, aLb / bUb, aUb / bLb, aUb / bUb};

  const auto [lb, ub] = std::minmax_element(vals.begin(), vals.end());

  engine.updateBounds(_y, *lb, *ub);
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
