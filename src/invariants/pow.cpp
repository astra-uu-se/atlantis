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

  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::min();

  for (const Int aBound : std::array<Int, 2>{aLb, aUb}) {
    for (const Int bBound : std::array<Int, 2>{bLb, bUb}) {
      if (aBound != 0 || bBound >= 0) {
        lb = std::min<Int>(lb, std::pow(aBound, bBound));
        ub = std::max<Int>(ub, std::pow(aBound, bBound));
      } else {
        if (aLb < 0) {
          lb = std::min<Int>(lb, std::pow(-1, bBound));
          ub = std::max<Int>(ub, std::pow(-1, bBound));
        }
        if (aUb > 0) {
          lb = std::min<Int>(lb, std::pow(1, bBound));
          ub = std::max<Int>(ub, std::pow(1, bBound));
        }
      }
    }
  }

  _zeroReplacement = 1;
  // If base can be 0:
  if (aLb <= 0 && 0 <= aUb) {
    lb = std::min<Int>(lb, 0);
    ub = std::max<Int>(ub, 0);

    // If base can be 0 and exponent can be negative:
    if (bLb < 0) {
      // x^-1 is in {-1, 0, 1}
      lb = std::min<Int>(lb, 1);
      ub = std::max<Int>(ub, 1);
      // can be -1 if base is negative
      if (aLb < 0) {
        lb = std::min<Int>(lb, -1);
        ub = std::max<Int>(ub, -1);
      }
      if (aUb < 0) {
        _zeroReplacement = -1;
      }
    }
  }

  if (aLb < 0 && bUb >= 2) {
    lb = std::min<Int>(lb, std::pow(aLb, bUb - (bUb % 2 == 0)));
    ub = std::max<Int>(ub, std::pow(aLb, bUb - (bUb % 2 == 1)));
  }

  engine.updateBounds(_y, lb, ub);
}

void Pow::recompute(Timestamp ts, Engine& engine) {
  const Int aVal = engine.value(ts, _a);
  const Int bVal = engine.value(ts, _b);
  if (aVal == 0 && bVal < 0) {
    updateValue(ts, engine, _y, std::pow(_zeroReplacement, bVal));
    return;
  }
  updateValue(ts, engine, _y, std::pow(aVal, bVal));
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
