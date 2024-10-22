#include "atlantis/propagation/invariants/linear.hpp"

#include <utility>

namespace atlantis::propagation {

Linear::Linear(SolverBase& solver, VarId output, std::vector<Int>&& coeffs,
               std::vector<VarViewId>&& varArray)
    : Invariant(solver),
      _output(output),
      _coeffs(std::move(coeffs)),
      _varArray(std::move(varArray)) {}

Linear::Linear(SolverBase& solver, VarViewId output, std::vector<Int>&& coeffs,
               std::vector<VarViewId>&& varArray)
    : Linear(solver, VarId(output), std::move(coeffs), std::move(varArray)) {
  assert(output.isVar());
}

Linear::Linear(SolverBase& solver, VarId output,
               std::vector<VarViewId>&& varArray)
    : Linear(solver, output, std::vector<Int>(varArray.size(), 1),
             std::move(varArray)) {}

Linear::Linear(SolverBase& solver, VarViewId output,
               std::vector<VarViewId>&& varArray)
    : Linear(solver, VarId(output), std::move(varArray)) {
  assert(output.isVar());
}

void Linear::registerVars() {
  // precondition: this invariant must be registered with the solver before it
  // is initialised.
  assert(_id != NULL_ID);

  for (size_t i = 0; i < _varArray.size(); ++i) {
    _solver.registerInvariantInput(_id, _varArray[i], i, false);
  }
  registerDefinedVar(_output);
}

void Linear::updateBounds(bool widenOnly) {
  // precondition: this invariant must be registered with the solver before it
  // is initialised.
  Int sumLb = 0;
  Int sumUb = 0;
  for (size_t i = 0; i < _varArray.size(); ++i) {
    const Int v1 = _coeffs[i] * _solver.lowerBound(_varArray[i]);
    const Int v2 = _coeffs[i] * _solver.upperBound(_varArray[i]);
    sumLb += std::min(v1, v2);
    sumUb += std::max(v1, v2);
  }
  _solver.updateBounds(_output, sumLb, sumUb, widenOnly);
}

void Linear::recompute(Timestamp ts) {
  Int sum = 0;
  for (size_t i = 0; i < _varArray.size(); ++i) {
    sum += _coeffs[i] * _solver.value(ts, _varArray[i]);
  }
  updateValue(ts, _output, sum);
}

void Linear::notifyInputChanged(Timestamp ts, LocalId id) {
  assert(id < _varArray.size());
  incValue(ts, _output,
           (_solver.value(ts, _varArray[id]) -
            _solver.committedValue(_varArray[id])) *
               _coeffs[id]);
}

VarViewId Linear::nextInput(Timestamp ts) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  assert(0 <= _state.value(ts));
  if (index < _varArray.size()) {
    return _varArray[index];
  }
  return NULL_ID;  // Done
}

void Linear::notifyCurrentInputChanged(Timestamp ts) {
  assert(_state.value(ts) != -1);
  notifyInputChanged(ts, _state.value(ts));
}

}  // namespace atlantis::propagation
