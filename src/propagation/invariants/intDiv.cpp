#include "atlantis/propagation/invariants/intDiv.hpp"

#include <algorithm>
#include <limits>
#include <vector>

namespace atlantis::propagation {

IntDiv::IntDiv(SolverBase& solver, VarId output, VarViewId numerator,
               VarViewId denominator)
    : Invariant(solver),
      _output(output),
      _numerator(numerator),
      _denominator(denominator) {}

IntDiv::IntDiv(SolverBase& solver, VarViewId output, VarViewId numerator,
               VarViewId denominator)
    : IntDiv(solver, VarId(output), numerator, denominator) {
  assert(output.isVar());
}

void IntDiv::registerVars() {
  assert(_id != NULL_ID);
  _solver.registerInvariantInput(_id, _numerator, 0, false);
  _solver.registerInvariantInput(_id, _denominator, 0, false);
  registerDefinedVar(_output);
}

void IntDiv::updateBounds(bool widenOnly) {
  const Int nomLb = _solver.lowerBound(_numerator);
  const Int nomUb = _solver.upperBound(_numerator);
  const Int denLb = _solver.lowerBound(_denominator);
  const Int denUb = _solver.upperBound(_denominator);

  assert(denLb != 0 || denUb != 0);

  std::vector<Int> denominators;
  denominators.reserve(4);

  if (denLb <= 0 && 0 < denUb) {
    denominators.emplace_back(1);
  }
  if (denLb < 0 && 0 <= denUb) {
    denominators.emplace_back(-1);
  }
  if (denLb != 0) {
    denominators.emplace_back(denLb);
  }
  if (denUb != 0) {
    denominators.emplace_back(denUb);
  }

  assert(!denominators.empty());
  Int outLb = std::numeric_limits<Int>::max();
  Int outUb = std::numeric_limits<Int>::min();
  for (const Int d : denominators) {
    outLb = std::min(outLb, std::min(nomLb / d, nomUb / d));
    outUb = std::max(outUb, std::max(nomLb / d, nomUb / d));
  }

  _solver.updateBounds(_output, outLb, outUb, widenOnly);
}

void IntDiv::close(Timestamp) {
  assert(_id != NULL_ID);

  const Int denLb = _solver.lowerBound(_denominator);
  const Int denUb = _solver.upperBound(_denominator);

  assert(denLb != 0 || denUb != 0);
  _zeroReplacement = (denLb < 0 && denUb <= 0) ? -1 : 1;
}

void IntDiv::recompute(Timestamp ts) {
  assert(_zeroReplacement != 0);
  const Int denominator = _solver.value(ts, _denominator);
  updateValue(ts, _output,
              _solver.value(ts, _numerator) /
                  (denominator != 0 ? denominator : _zeroReplacement));
}

VarViewId IntDiv::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _numerator;
    case 1:
      return _denominator;
    default:
      return NULL_ID;
  }
}

void IntDiv::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }

void IntDiv::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }
}  // namespace atlantis::propagation
