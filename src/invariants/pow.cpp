#include "invariants/pow.hpp"

#include <cmath>

#include "core/engine.hpp"

Pow::Pow(VarId output, VarId x, VarId y)
    : Invariant(), _output(output), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void Pow::registerVars(Engine& engine) {
  assert(!_id.equals(NULL_ID));

  engine.registerInvariantInput(_id, _x, 0);
  engine.registerInvariantInput(_id, _y, 0);
  registerDefinedVariable(engine, _output);
}

void Pow::updateBounds(Engine& engine, bool widenOnly) {
  const Int xLb = engine.lowerBound(_x);
  const Int xUb = engine.upperBound(_x);

  const Int yLb = engine.lowerBound(_y);
  const Int yUb = engine.upperBound(_y);

  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::min();

  for (const Int aBound : std::array<Int, 2>{xLb, xUb}) {
    for (const Int bBound : std::array<Int, 2>{yLb, yUb}) {
      if (aBound != 0 || bBound >= 0) {
        lb = std::min<Int>(lb, std::pow(aBound, bBound));
        ub = std::max<Int>(ub, std::pow(aBound, bBound));
      } else {
        if (xLb < 0) {
          lb = std::min<Int>(lb, std::pow(-1, bBound));
          ub = std::max<Int>(ub, std::pow(-1, bBound));
        }
        if (xUb > 0) {
          lb = std::min<Int>(lb, std::pow(1, bBound));
          ub = std::max<Int>(ub, std::pow(1, bBound));
        }
      }
    }
  }

  _zeroReplacement = 1;
  // If base can be 0:
  if (xLb <= 0 && 0 <= xUb) {
    lb = std::min<Int>(lb, 0);
    ub = std::max<Int>(ub, 0);

    // If base can be 0 and exponent can be negative:
    if (yLb < 0) {
      // x^-1 is in {-1, 0, 1}
      lb = std::min<Int>(lb, 1);
      ub = std::max<Int>(ub, 1);
      // can be -1 if base is negative
      if (xLb < 0) {
        lb = std::min<Int>(lb, -1);
        ub = std::max<Int>(ub, -1);
      }
      if (xUb < 0) {
        _zeroReplacement = -1;
      }
    }
  }

  if (xLb < 0 && yUb >= 2) {
    lb = std::min<Int>(lb, std::pow(xLb, yUb - (yUb % 2 == 0)));
    ub = std::max<Int>(ub, std::pow(xLb, yUb - (yUb % 2 == 1)));
  }

  engine.updateBounds(_output, lb, ub, widenOnly);
}

void Pow::recompute(Timestamp ts, Engine& engine) {
  const Int xVal = engine.value(ts, _x);
  const Int yVal = engine.value(ts, _y);
  if (xVal == 0 && yVal < 0) {
    updateValue(ts, engine, _output, std::pow(_zeroReplacement, yVal));
    return;
  }
  updateValue(ts, engine, _output, std::pow(xVal, yVal));
}

VarId Pow::nextInput(Timestamp ts, Engine&) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
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
