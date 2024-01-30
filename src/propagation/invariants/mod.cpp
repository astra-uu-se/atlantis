#include "propagation/invariants/mod.hpp"

namespace atlantis::propagation {

Mod::Mod(SolverBase& solver, VarId output, VarId nominator, VarId denominator)
    : Invariant(solver), _output(output), _nominator(nominator), _denominator(denominator) {
  _modifiedVars.reserve(1);
}

void Mod::registerVars() {
  assert(!_id.equals(NULL_ID));
  _solver.registerInvariantInput(_id, _nominator, 0, false);
  _solver.registerInvariantInput(_id, _denominator, 0, false);
  registerDefinedVar(_output);
}

void Mod::updateBounds(bool widenOnly) {
  _solver.updateBounds(_output, std::min(Int(0), _solver.lowerBound(_nominator)),
                       std::max(Int(0), _solver.upperBound(_nominator)), widenOnly);
}

void Mod::close(Timestamp) {
  assert(_solver.lowerBound(_denominator) != 0 || _solver.upperBound(_denominator) != 0);
  if (_solver.lowerBound(_denominator) <= 0 && 0 <= _solver.upperBound(_denominator)) {
    _zeroReplacement = _solver.upperBound(_denominator) >= 1 ? 1 : -1;
  }
}

void Mod::recompute(Timestamp ts) {
  assert(_zeroReplacement != 0);
  const Int denominator = _solver.value(ts, _denominator);
  updateValue(ts, _output,
              _solver.value(ts, _nominator) %
                  std::abs(denominator != 0 ? denominator : _zeroReplacement));
}

void Mod::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }

VarId Mod::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _nominator;
    case 1:
      return _denominator;
    default:
      return NULL_ID;
  }
}

void Mod::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }
}  // namespace atlantis::propagation