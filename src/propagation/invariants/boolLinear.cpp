#include "atlantis/propagation/invariants/boolLinear.hpp"

#include <utility>
#include <vector>

#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/utils/overflow.hpp"

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
    _solver.registerInvariantInput(_id, _violArray[i], i, false);
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
    // violation == 0 <=> true
    const Int boolLb = violUb == 0 ? 1 : 0;
    const Int boolUb = violLb == 0 ? 1 : 0;
    assert(0 <= boolLb);
    assert(boolLb <= boolUb);
    assert(boolUb <= 1);

    lb += _coeffs[i] * (_coeffs[i] < 0 ? boolUb : boolLb);
    ub += _coeffs[i] * (_coeffs[i] < 0 ? boolLb : boolUb);
  }
  _solver.updateBounds(_output, lb, ub, widenOnly);
}

void BoolLinear::recompute(Timestamp ts) {
  Int totalSum = 0;
  for (size_t i = 0; i < _violArray.size(); ++i) {
    Int prod;
    const Int val = _solver.value(ts, _violArray[i]) == 0 ? 1 : 0;
    if (mul_overflow<Int>(_coeffs[i], val, prod)) {
      totalSum = _coeffs[i] > 0 ? std::numeric_limits<Int>::max()
                                : std::numeric_limits<Int>::min();
      break;
    }
    Int sum;
    if (add_overflow<Int>(totalSum, prod, sum)) {
      totalSum = prod < 0 ? std::numeric_limits<Int>::min()
                          : std::numeric_limits<Int>::max();
      break;
    }
    totalSum = sum;
  }
  updateValue(ts, _output, totalSum);
}

void BoolLinear::notifyInputChanged(const Timestamp ts, const LocalId id) {
  assert(id < _violArray.size());
  const Int newValue = _solver.value(ts, _violArray[id]) == 0 ? 1 : 0;
  const Int committedValue =
      _solver.committedValue(_violArray[id]) == 0 ? 1 : 0;
  if (newValue == committedValue) {
    return;
  }

  Int prod;
  const Int difference = newValue - committedValue;
  if (mul_overflow<Int>(_coeffs[id], difference, prod)) {
    updateValue(ts, _output,
                (_coeffs[id] < 0) == (difference < 0)
                    ? std::numeric_limits<Int>::max()
                    : std::numeric_limits<Int>::min());
  }
  Int sum;
  if (add_overflow<Int>(_solver.value(ts, _output), prod, sum)) {
    updateValue(ts, _output,
                prod < 0 ? std::numeric_limits<Int>::min()
                         : std::numeric_limits<Int>::max());
    return;
  }
  updateValue(ts, _output, sum);
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
