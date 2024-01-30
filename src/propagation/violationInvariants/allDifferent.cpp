#include "propagation/violationInvariants/allDifferent.hpp"

namespace atlantis::propagation {

/**
 * @param violationId id for the violationCount
 */
AllDifferent::AllDifferent(SolverBase& solver, VarId violationId,
                           std::vector<VarId>&& vars)
    : ViolationInvariant(solver, violationId),
      _vars(std::move(vars)),
      _committedValues(_vars.size(), 0),
      _counts(),
      _offset(0) {
  _modifiedVars.reserve(_vars.size());
}

void AllDifferent::registerVars() {
  assert(!_id.equals(NULL_ID));
  for (size_t i = 0; i < _vars.size(); ++i) {
    _solver.registerInvariantInput(_id, _vars[i], i, false);
  }
  registerDefinedVar(_violationId);
}

void AllDifferent::updateBounds(bool widenOnly) {
  _solver.updateBounds(_violationId, 0, static_cast<Int>(_vars.size() - 1), widenOnly);
}

void AllDifferent::close(Timestamp ts) {
  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::min();

  for (size_t i = 0; i < _vars.size(); ++i) {
    lb = std::min(lb, _solver.lowerBound(_vars[i]));
    ub = std::max(ub, _solver.upperBound(_vars[i]));
  }
  assert(ub >= lb);
  _counts.resize(static_cast<unsigned long>(ub - lb + 1),
                 CommittableInt(ts, 0));
  _offset = lb;
}

void AllDifferent::recompute(Timestamp ts) {
  for (CommittableInt& c : _counts) {
    c.setValue(ts, 0);
  }

  Int violInc = 0;
  for (size_t i = 0; i < _vars.size(); ++i) {
    violInc += increaseCount(ts, _solver.value(ts, _vars[i]));
  }
  updateValue(ts, _violationId, violInc);
}

void AllDifferent::notifyInputChanged(Timestamp ts, LocalId id) {
  assert(id < _committedValues.size());
  const Int newValue = _solver.value(ts, _vars[id]);
  if (newValue == _committedValues[id]) {
    return;
  }
  incValue(ts, _violationId,
           static_cast<Int>(decreaseCount(ts, _committedValues[id]) +
                            increaseCount(ts, newValue)));
}

VarId AllDifferent::nextInput(Timestamp ts) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  if (index < _vars.size()) {
    return _vars[index];
  }
  return NULL_ID;
}

void AllDifferent::notifyCurrentInputChanged(Timestamp ts) {
  assert(static_cast<size_t>(_state.value(ts)) < _vars.size());
  notifyInputChanged(ts, static_cast<size_t>(_state.value(ts)));
}

void AllDifferent::commit(Timestamp ts) {
  Invariant::commit(ts);

  for (size_t i = 0; i < _committedValues.size(); ++i) {
    _committedValues[i] = _solver.committedValue(_vars[i]);
  }

  for (CommittableInt& committableInt : _counts) {
    committableInt.commitIf(ts);
  }
}

}  // namespace atlantis::propagation