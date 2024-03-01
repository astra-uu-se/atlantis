#include "atlantis/propagation/invariants/count.hpp"

#include <limits>
#include <utility>

namespace atlantis::propagation {

Count::Count(SolverBase& solver, VarId output, VarId y,
             std::vector<VarId>&& varArray)
    : Invariant(solver),
      _output(output),
      _y(y),
      _vars(std::move(varArray)),
      _committedValues(_vars.size(), 0),
      _counts(),
      _offset(0) {
  _modifiedVars.reserve(_vars.size() + 1);
}

void Count::registerVars() {
  assert(_id != NULL_ID);
  for (size_t i = 0; i < _vars.size(); ++i) {
    _solver.registerInvariantInput(_id, _vars[i], i, false);
  }
  _solver.registerInvariantInput(_id, _y, _vars.size(), false);
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
  lb = std::max(lb, _solver.lowerBound(_y));
  ub = std::max(ub, _solver.lowerBound(_y));

  _counts.resize(static_cast<unsigned long>(ub - lb + 1),
                 CommittableInt(ts, 0));
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
  updateValue(ts, _output, count(ts, _solver.value(ts, _y)));
}

void Count::notifyInputChanged(Timestamp ts, LocalId id) {
  if (id == _committedValues.size()) {
    updateValue(ts, _output, count(ts, _solver.value(ts, _y)));
    return;
  }
  assert(id < _committedValues.size());
  const Int newValue = _solver.value(ts, _vars[id]);
  if (newValue == _committedValues[id]) {
    return;
  }
  decreaseCount(ts, _committedValues[id]);
  increaseCount(ts, newValue);
  updateValue(ts, _output, count(ts, _solver.value(ts, _y)));
}

VarId Count::nextInput(Timestamp ts) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  if (index < _vars.size()) {
    return _vars[index];
  } else if (index == _vars.size()) {
    return _y;
  }
  return NULL_ID;
}

void Count::notifyCurrentInputChanged(Timestamp ts) {
  assert(static_cast<size_t>(_state.value(ts)) <= _vars.size());
  notifyInputChanged(ts, static_cast<size_t>(_state.value(ts)));
}

void Count::commit(Timestamp ts) {
  Invariant::commit(ts);

  for (size_t i = 0; i < _committedValues.size(); ++i) {
    _committedValues[i] = _solver.committedValue(_vars[i]);
  }

  for (CommittableInt& committableInt : _counts) {
    committableInt.commitIf(ts);
  }
}
}  // namespace atlantis::propagation
