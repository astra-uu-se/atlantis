#include "atlantis/propagation/invariants/intDiv.hpp"

#include <algorithm>
#include <limits>
#include <vector>

namespace atlantis::propagation {

IntDiv::IntDiv(SolverBase& solver, VarId output, VarId nominator,
               VarId denominator)
    : Invariant(solver),
      _output(output),
      _nominator(nominator),
      _denominator(denominator) {
  _modifiedVars.reserve(1);
}

void IntDiv::registerVars() {
  assert(_id != NULL_ID);
  _solver.registerInvariantInput(_id, _nominator, 0, false);
  _solver.registerInvariantInput(_id, _denominator, 0, false);
  registerDefinedVar(_output);
}

void IntDiv::updateBounds(bool widenOnly) {
  const Int nomLb = _solver.lowerBound(_nominator);
  const Int nomUb = _solver.upperBound(_nominator);
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
  _solver.registerInvariantInput(_id, _nominator, 0, false);
  _solver.registerInvariantInput(_id, _denominator, 0, false);

  const Int denLb = _solver.lowerBound(_denominator);
  const Int denUb = _solver.upperBound(_denominator);

  assert(denLb != 0 || denUb != 0);
  if (denLb <= 0 && 0 <= denUb) {
    _zeroReplacement = denUb >= 1 ? 1 : -1;
  }
}

void IntDiv::recompute(Timestamp ts) {
  assert(_zeroReplacement != 0);
  const Int denominator = _solver.value(ts, _denominator);
  updateValue(ts, _output,
              _solver.value(ts, _nominator) /
                  (denominator != 0 ? denominator : _zeroReplacement));
}

VarId IntDiv::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _nominator;
    case 1:
      return _denominator;
    default:
      return NULL_ID;
  }
}

void IntDiv::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }

void IntDiv::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }
}  // namespace atlantis::propagation
