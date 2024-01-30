#include "propagation/invariants/pow.hpp"

namespace atlantis::propagation {

Pow::Pow(SolverBase& solver, VarId output, VarId base, VarId exponent)
    : Invariant(solver), _output(output), _base(base), _exponent(exponent) {
  _modifiedVars.reserve(1);
}

void Pow::registerVars() {
  assert(!_id.equals(NULL_ID));

  _solver.registerInvariantInput(_id, _base, 0, false);
  _solver.registerInvariantInput(_id, _exponent, 0, false);
  registerDefinedVar(_output);
}

void Pow::updateBounds(bool widenOnly) {
  const Int baseLb = _solver.lowerBound(_base);
  const Int baseUb = _solver.upperBound(_base);

  const Int expLb = _solver.lowerBound(_exponent);
  const Int expUb = _solver.upperBound(_exponent);

  Int outLb = std::numeric_limits<Int>::max();
  Int outUb = std::numeric_limits<Int>::min();

  for (const Int baseBound : std::array<Int, 2>{baseLb, baseUb}) {
    for (const Int expBound : std::array<Int, 2>{expLb, expUb}) {
      if (baseBound != 0 || expBound >= 0) {
        outLb = std::min<Int>(outLb, static_cast<Int>(std::pow(baseBound, expBound)));
        outUb = std::max<Int>(outUb, static_cast<Int>(std::pow(baseBound, expBound)));
      } else {
        if (baseLb < 0) {
          outLb = std::min<Int>(outLb, static_cast<Int>(std::pow(-1, expBound)));
          outUb = std::max<Int>(outUb, static_cast<Int>(std::pow(-1, expBound)));
        }
        if (baseUb > 0) {
          outLb = std::min<Int>(outLb, static_cast<Int>(std::pow(1, expBound)));
          outUb = std::max<Int>(outUb, static_cast<Int>(std::pow(1, expBound)));
        }
      }
    }
  }

  _zeroReplacement = 1;
  // If base can be 0:
  if (baseLb <= 0 && 0 <= baseUb) {
    outLb = std::min<Int>(outLb, 0);
    outUb = std::max<Int>(outUb, 0);

    // If base can be 0 and exponent can be negative:
    if (expLb < 0) {
      // x^-1 is in {-1, 0, 1}
      outLb = std::min<Int>(outLb, 1);
      outUb = std::max<Int>(outUb, 1);
      // can be -1 if base is negative
      if (baseLb < 0) {
        outLb = std::min<Int>(outLb, -1);
        outUb = std::max<Int>(outUb, -1);
      }
      if (baseUb < 0) {
        _zeroReplacement = -1;
      }
    }
  }

  if (baseLb < 0 && expUb >= 2) {
    outLb = std::min<Int>(
        outLb, static_cast<Int>(std::pow(baseLb, expUb - (expUb % 2 == 0))));
    outUb = std::max<Int>(
        outUb, static_cast<Int>(std::pow(baseLb, expUb - (expUb % 2 == 1))));
  }

  _solver.updateBounds(_output, outLb, outUb, widenOnly);
}

void Pow::recompute(Timestamp ts) {
  const Int baseVal = _solver.value(ts, _base);
  const Int expVal = _solver.value(ts, _exponent);
  if (baseVal == 0 && expVal < 0) {
    updateValue(ts, _output, static_cast<Int>(std::pow(_zeroReplacement, expVal)));
    return;
  }
  updateValue(ts, _output, static_cast<Int>(std::pow(baseVal, expVal)));
}

VarId Pow::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _base;
    case 1:
      return _exponent;
    default:
      return NULL_ID;
  }
}

void Pow::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }

void Pow::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }
}  // namespace atlantis::propagation