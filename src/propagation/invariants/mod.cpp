#include "atlantis/propagation/invariants/mod.hpp"

#include <cmath>

namespace atlantis::propagation {

Mod::Mod(SolverBase& solver, VarId output, VarViewId numerator,
         VarViewId denominator)
    : Invariant(solver),
      _output(output),
      _numerator(numerator),
      _denominator(denominator) {}

Mod::Mod(SolverBase& solver, VarViewId output, VarViewId numerator,
         VarViewId denominator)
    : Mod(solver, VarId(output), numerator, denominator) {
  assert(output.isVar());
}

void Mod::registerVars() {
  assert(_id != NULL_ID);
  _solver.registerInvariantInput(_id, _numerator, 0, false);
  _solver.registerInvariantInput(_id, _denominator, 0, false);
  registerDefinedVar(_output);
}

void Mod::updateBounds(bool widenOnly) {
  _solver.updateBounds(
      _output, std::min(Int(0), _solver.lowerBound(_numerator)),
      std::max(Int(0), _solver.upperBound(_numerator)), widenOnly);
}

void Mod::close(Timestamp) {
  assert(_solver.lowerBound(_denominator) != 0 ||
         _solver.upperBound(_denominator) != 0);
}

void Mod::recompute(Timestamp ts) {
  const Int denominator = _solver.value(ts, _denominator);
  updateValue(ts, _output,
              _solver.value(ts, _numerator) %
                  std::abs(denominator != 0 ? denominator : 1));
}

void Mod::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }

VarViewId Mod::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _numerator;
    case 1:
      return _denominator;
    default:
      return NULL_ID;
  }
}

void Mod::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }
}  // namespace atlantis::propagation
