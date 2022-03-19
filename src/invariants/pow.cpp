#include "invariants/pow.hpp"

#include <cmath>

#include "core/engine.hpp"

void Pow::registerVars(Engine& engine) {
  assert(!_id.equals(NULL_ID));

  engine.registerInvariantInput(_id, _a, 0);
  engine.registerInvariantInput(_id, _b, 0);
  registerDefinedVariable(engine, _y);
}

void Pow::updateBounds(Engine& engine) {
  const Int aLb = engine.lowerBound(_a);
  const Int aUb = engine.upperBound(_a);
  const Int bLb = engine.lowerBound(_b);
  const Int bUb = engine.upperBound(_b);

  const Int lb = aLb >= 0 ? std::pow(aLb, bLb)
                          : std::pow(aLb, std::max(bLb, bUb - (bUb % 2 == 0)));

  const Int ub = std::max(static_cast<Int>(std::pow(aUb, bUb)),
                          std::max(bLb, bUb - (bUb % 2 == 0)));

  engine.updateBounds(_y, lb, ub);
}

void Pow::recompute(Timestamp ts, Engine& engine) {
  updateValue(ts, engine, _y,
              std::pow(engine.value(ts, _a), engine.value(ts, _b)));
}

VarId Pow::nextInput(Timestamp ts, Engine&) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _a;
    case 1:
      return _b;
    default:
      return NULL_ID;
  }
}

void Pow::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void Pow::notifyInputChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

void Pow::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}
