#include "atlantis/propagation/invariants/linear.hpp"

#include <limits>
#include <utility>

#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/utils/overflow.hpp"

namespace atlantis::propagation {

Linear::Linear(SolverBase& solver, const VarId output, std::vector<Int>&& coeffs,
               std::vector<VarViewId>&& varArray, const Int outputOffset)
    : Invariant(solver),
      _outputOffset(outputOffset),
      _output(output),
      _coeffs(std::move(coeffs)),
      _varArray(std::move(varArray)) {}

Linear::Linear(SolverBase& solver, const VarViewId output, std::vector<Int>&& coeffs,
               std::vector<VarViewId>&& varArray, const Int outputOffset)
    : Linear(solver, VarId{output}, std::move(coeffs), std::move(varArray), outputOffset) {
  assert(output.isVar());
}

Linear::Linear(SolverBase& solver, const VarId output,
               std::vector<VarViewId>&& varArray, const Int outputOffset)
    : Linear(solver, output, std::vector<Int>(varArray.size(), 1),
             std::move(varArray), outputOffset) {}

Linear::Linear(SolverBase& solver, const VarViewId output,
               std::vector<VarViewId>&& varArray, const Int outputOffset)
    : Linear(solver, VarId{output}, std::move(varArray), outputOffset) {
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

void Linear::updateBounds(const bool widenOnly) {
  Int sumLb = _outputOffset;
  Int sumUb = _outputOffset;
  for (size_t i = 0; i < _varArray.size(); ++i) {
    const Int varLb = _solver.lowerBound(_varArray[i]);
    const Int varUb = _solver.upperBound(_varArray[i]);
    const Int prod1 = overflow::saturatingMul(_coeffs[i], varLb);
    const Int prod2 = overflow::saturatingMul(_coeffs[i], varUb);
    sumLb = overflow::saturatingAdd(sumLb, std::min(prod1, prod2));
    sumUb = overflow::saturatingAdd(sumUb, std::max(prod1, prod2));
    assert(sumLb <= sumUb);
  }
  _solver.updateBounds(_output, sumLb, sumUb, widenOnly);
}

void Linear::recompute(const Timestamp ts) {
  Int total = _outputOffset;
  for (size_t i = 0; i < _varArray.size(); ++i) {
    total += _coeffs[i] * _solver.value(ts, _varArray[i]);
  }
  updateValue(ts, _output, total);
}

void Linear::notifyInputChanged(const Timestamp ts, const LocalId id) {
  assert(id < _varArray.size());
  const Int committedValue = _solver.committedValue(_varArray[id]);
  const Int newValue = _solver.value(ts, _varArray[id]);
  if (newValue == committedValue) {
    return;
  }
  assert([&]() {
    Int diff;
    return !overflow::subOverflow(newValue, committedValue, &diff);
  }());

  assert([&]() {
    const Int diff = overflow::saturatingSub(newValue, committedValue);
    Int prod;
    return !overflow::mulOverflow(_coeffs.at(id), diff, &prod);
  }());

  assert([&]() {
    const Int diff = overflow::saturatingSub(newValue, committedValue);
    const Int prod = overflow::saturatingMul(_coeffs.at(id), diff);
    Int sum;
    return !overflow::addOverflow(_solver.value(ts, _output), prod, &sum);
  }());

  incValue(ts, _output, _coeffs[id] * (newValue - committedValue));
}

VarViewId Linear::nextInput(const Timestamp ts) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  assert(0 <= _state.value(ts));
  if (index < _varArray.size()) {
    return _varArray[index];
  }
  return NULL_ID;  // Done
}

void Linear::notifyCurrentInputChanged(const Timestamp ts) {
  assert(_state.value(ts) != -1);
  notifyInputChanged(ts, _state.value(ts));
}

}  // namespace atlantis::propagation
