#include "atlantis/propagation/invariants/boolLinear.hpp"

#include <utility>
#include <vector>

namespace atlantis::propagation {

BoolLinear::BoolLinear(SolverBase& solver, VarId output,
                       std::vector<Int>&& coeffs,
                       std::vector<VarViewId>&& violArray)
    : Invariant(solver),
      _output(output),
      _coeffs(std::move(coeffs)),
      _violArray(std::move(violArray)) {}

BoolLinear::BoolLinear(SolverBase& solver, VarViewId output,
                       std::vector<Int>&& coeffs,
                       std::vector<VarViewId>&& violArray)
    : BoolLinear(solver, VarId(output), std::move(coeffs),
                 std::move(violArray)) {
  assert(output.isVar());
}

BoolLinear::BoolLinear(SolverBase& solver, VarId output,
                       std::vector<VarViewId>&& violArray)
    : BoolLinear(solver, output, std::vector<Int>(violArray.size(), 1),
                 std::move(violArray)) {}

BoolLinear::BoolLinear(SolverBase& solver, VarViewId output,
                       std::vector<VarViewId>&& violArray)
    : BoolLinear(solver, VarId(output), std::vector<Int>(violArray.size(), 1),
                 std::move(violArray)) {
  assert(output.isVar());
}

void BoolLinear::registerVars() {
  // precondition: this invariant must be registered with the solver before it
  // is initialised.
  assert(_id != NULL_ID);

  for (size_t i = 0; i < _violArray.size(); ++i) {
    _solver.registerInvariantInput(_id, _violArray[i], false);
  }
  registerDefinedVar(_output);
}

void BoolLinear::updateBounds(bool widenOnly) {
  // precondition: this invariant must be registered with the solver before it
  // is initialised.
  Int lb = 0;
  Int ub = 0;
  for (size_t i = 0; i < _violArray.size(); ++i) {
    const Int violLb = _solver.lowerBound(_violArray[i]);
    const Int violUb = _solver.upperBound(_violArray[i]);
    // violation != 0 <=> false
    const Int boolLb = static_cast<Int>(violLb == 0 && violUb == 0);
    // violation == 0 <=> true
    const Int boolUb = static_cast<Int>(violLb <= 0 && 0 <= violUb);
    assert(0 <= boolLb);
    assert(boolLb <= boolUb);
    assert(boolUb <= 1);

    lb += _coeffs[i] * (_coeffs[i] < 0 ? boolUb : boolLb);
    ub += _coeffs[i] * (_coeffs[i] < 0 ? boolLb : boolUb);
  }
  _solver.updateBounds(_output, lb, ub, widenOnly);
}

void BoolLinear::recompute(Timestamp ts) {
  Int sum = 0;
  for (size_t i = 0; i < _violArray.size(); ++i) {
    sum += _coeffs[i] * static_cast<Int>(_solver.value(ts, _violArray[i]) == 0);
  }
  updateValue(ts, _output, sum);
}

void BoolLinear::notifyInputChanged(Timestamp ts, LocalId id) {
  assert(id < _violArray.size());
  const Int newValue = static_cast<Int>(_solver.value(ts, _violArray[id]) == 0);
  const Int committedValue =
      static_cast<Int>(_solver.committedValue(_violArray[id]) == 0);
  if (newValue == committedValue) {
    return;
  }
  incValue(ts, _output, (newValue - committedValue) * _coeffs[id]);
}

VarViewId BoolLinear::nextInput(Timestamp ts) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  assert(0 <= _state.value(ts));
  if (index < _violArray.size()) {
    return _violArray[index];
  }
  return NULL_ID;  // Done
}

void BoolLinear::notifyCurrentInputChanged(Timestamp ts) {
  assert(_state.value(ts) != -1);
  notifyInputChanged(ts, _state.value(ts));
}

}  // namespace atlantis::propagation
