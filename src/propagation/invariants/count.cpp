#include "atlantis/propagation/invariants/count.hpp"

#include <limits>
#include <utility>

#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::propagation {

Count::Count(SolverBase& solver, VarId output, VarViewId needle,
             std::vector<VarViewId>&& varArray)
    : Invariant(solver),
      _output(output),
      _needle(needle),
      _vars(std::move(varArray)),
      _offset(0) {}

Count::Count(SolverBase& solver, VarViewId output, VarViewId needle,
             std::vector<VarViewId>&& varArray)
    : Count(solver, VarId(output), needle, std::move(varArray)) {
  assert(output.isVar());
}

inline void Count::increaseCount(Timestamp ts, Int value) {
  if (value - _offset < 0 ||
      static_cast<Int>(_counts.size()) <= value - _offset) {
    return;
  }
  assert(_counts[value - _offset].value(ts) + 1 > 0);
  assert(_counts[value - _offset].value(ts) + 1 <=
         static_cast<Int>(_vars.size()));
  _counts[value - _offset].incValue(ts, 1);
}

inline void Count::decreaseCount(Timestamp ts, Int value) {
  if (value - _offset < 0 ||
      static_cast<Int>(_counts.size()) <= value - _offset) {
    return;
  }
  assert(_counts[value - _offset].value(ts) - 1 >= 0);
  assert(_counts[value - _offset].value(ts) - 1 <
         static_cast<Int>(_vars.size()));
  _counts[value - _offset].incValue(ts, -1);
}

inline signed char Count::count(Timestamp ts, Int value) const {
  if (value - _offset < 0 ||
      static_cast<Int>(_counts.size()) <= value - _offset) {
    return 0;
  }
  assert(0 <= value - _offset);
  assert(static_cast<size_t>(value - _offset) <= _counts.size());
  assert(_counts.at(value - _offset).value(ts) >= 0);
  return static_cast<signed char>(_counts[value - _offset].value(ts));
}

void Count::registerVars() {
  assert(_id != NULL_ID);
  for (size_t i = 0; i < _vars.size(); ++i) {
    _solver.registerInvariantInput(_id, _vars[i], i, false);
  }
  _solver.registerInvariantInput(_id, _needle, _vars.size(), false);
  registerDefinedVar(_output);
}

void Count::updateBounds(bool widenOnly) {
  _solver.updateBounds(_output, 0, static_cast<Int>(_vars.size()), widenOnly);
}

void Count::close(Timestamp ts) {
  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::min();

  for (const auto& var : _vars) {
    lb = std::min(lb, _solver.lowerBound(var));
    ub = std::max(ub, _solver.upperBound(var));
  }
  assert(ub >= lb);
  lb = std::max(lb, _solver.lowerBound(_needle));
  ub = std::max(ub, _solver.lowerBound(_needle));

  _counts.resize(static_cast<size_t>(ub - lb + 1), CommittableInt(ts, 0));
  _offset = lb;
}

void Count::recompute(Timestamp ts) {
  for (CommittableInt& c : _counts) {
    c.setValue(ts, 0);
  }

  updateValue(ts, _output, 0);

  for (const auto& var : _vars) {
    increaseCount(ts, _solver.value(ts, var));
  }
  updateValue(ts, _output, count(ts, _solver.value(ts, _needle)));
}

void Count::notifyInputChanged(Timestamp ts, LocalId id) {
  if (id == _vars.size()) {
    updateValue(ts, _output, count(ts, _solver.value(ts, _needle)));
    return;
  }
  assert(id < _vars.size());
  const Int newValue = _solver.value(ts, _vars[id]);
  const Int committedValue = _solver.committedValue(_vars[id]);
  if (newValue == committedValue) {
    return;
  }
  decreaseCount(ts, committedValue);
  increaseCount(ts, newValue);
  updateValue(ts, _output, count(ts, _solver.value(ts, _needle)));
}

VarViewId Count::nextInput(Timestamp ts) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  if (index < _vars.size()) {
    return _vars[index];
  }
  if (index == _vars.size()) {
    return _needle;
  }
  return NULL_ID;
}

void Count::notifyCurrentInputChanged(Timestamp ts) {
  assert(static_cast<size_t>(_state.value(ts)) <= _vars.size());
  notifyInputChanged(ts, static_cast<size_t>(_state.value(ts)));
}

void Count::commit(Timestamp ts) {
  Invariant::commit(ts);

  for (CommittableInt& committableInt : _counts) {
    committableInt.commitIf(ts);
  }
}
}  // namespace atlantis::propagation
