#include "atlantis/propagation/violationInvariants/globalCardinalityLowUp.hpp"

#include <algorithm>

#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::propagation {

GlobalCardinalityLowUp::GlobalCardinalityLowUp(
    SolverBase& solver, VarId violationId, std::vector<VarViewId>&& vars,
    const std::vector<Int>& cover, const std::vector<Int>& lowerBounds,
    const std::vector<Int>& upperBounds)
    : ViolationInvariant(solver, violationId),
      _vars(std::move(vars)),
      _shortage(NULL_TIMESTAMP, 0),
      _excess(NULL_TIMESTAMP, 0),
      _offset(0) {
  assert(lowerBounds.size() == upperBounds.size() &&
         lowerBounds.size() == cover.size());

  const auto [lb, ub] = std::minmax_element(cover.begin(), cover.end());

  // a bound of -1 means that the count of a value is not restricted:
  _lowerBounds.assign(*ub - *lb + 3, -1);
  _upperBounds.assign(*ub - *lb + 3, -1);
  _offset = *lb - 1;

  for (size_t i = 0; i < cover.size(); ++i) {
    assert(lowerBounds[i] >= 0);
    assert(lowerBounds[i] <= upperBounds[i]);
    _lowerBounds[cover[i] - _offset] = lowerBounds[i];
    _upperBounds[cover[i] - _offset] = upperBounds[i];
  }
}

GlobalCardinalityLowUp::GlobalCardinalityLowUp(
    SolverBase& solver, VarViewId violationId, std::vector<VarViewId>&& vars,
    const std::vector<Int>& cover, const std::vector<Int>& lowerBounds,
    const std::vector<Int>& upperBounds)
    : GlobalCardinalityLowUp(solver, VarId(violationId), std::move(vars), cover,
                             lowerBounds, upperBounds) {
  assert(violationId.isVar());
}

GlobalCardinalityLowUp::GlobalCardinalityLowUp(SolverBase& solver,
                                               VarId violationId,
                                               std::vector<VarViewId>&& t_vars,
                                               const std::vector<Int>& cover,
                                               const std::vector<Int>& bounds)
    : GlobalCardinalityLowUp(solver, violationId, std::move(t_vars), cover,
                             bounds, bounds) {}

GlobalCardinalityLowUp::GlobalCardinalityLowUp(SolverBase& solver,
                                               VarViewId violationId,
                                               std::vector<VarViewId>&& t_vars,
                                               const std::vector<Int>& cover,
                                               const std::vector<Int>& bounds)
    : GlobalCardinalityLowUp(solver, VarId(violationId), std::move(t_vars),
                             cover, bounds) {
  assert(violationId.isVar());
}

signed char GlobalCardinalityLowUp::increaseCount(Timestamp ts, Int value) {
  const size_t pos = static_cast<size_t>(std::max<Int>(
      0, std::min(Int(_lowerBounds.size()) - 1, value - _offset)));
  if (_lowerBounds.at(pos) < 0) {
    return 0;
  }
  const Int newCount = _counts.at(pos).incValue(ts, 1);
  assert(newCount >= 0);
  assert(newCount <= static_cast<Int>(_vars.size()));
  return newCount > _upperBounds.at(pos)
             ? 1
             : (newCount > _lowerBounds.at(pos) ? 0 : -1);
}

signed char GlobalCardinalityLowUp::decreaseCount(Timestamp ts, Int value) {
  const size_t pos = static_cast<size_t>(std::max<Int>(
      0, std::min(Int(_lowerBounds.size()) - 1, value - _offset)));
  if (_lowerBounds.at(pos) < 0) {
    return 0;
  }

  const Int newCount = _counts.at(pos).incValue(ts, -1);
  assert(newCount >= 0);
  assert(newCount <= static_cast<Int>(_vars.size()));
  return newCount < _lowerBounds.at(pos)
             ? 1
             : (newCount < _upperBounds.at(pos) ? 0 : -1);
}

void GlobalCardinalityLowUp::registerVars() {
  assert(_id != NULL_ID);
  for (size_t i = 0; i < _vars.size(); ++i) {
    _solver.registerInvariantInput(_id, _vars[i], LocalId(i), false);
  }
  registerDefinedVar(_violationId);
}

void GlobalCardinalityLowUp::updateBounds(bool widenOnly) {
  Int shortage = 0;
  for (const Int lb : _lowerBounds) {
    shortage += lb;
  }
  Int excess = 0;
  for (const Int ub : _upperBounds) {
    excess = std::max(excess, static_cast<Int>(_vars.size()) - ub);
  }
  _solver.updateBounds(_violationId, 0, std::max(shortage, excess), widenOnly);
}

void GlobalCardinalityLowUp::close(Timestamp timestamp) {
  _counts.resize(_lowerBounds.size(), CommittableInt(timestamp, 0));
}

void GlobalCardinalityLowUp::recompute(Timestamp timestamp) {
  for (CommittableInt& c : _counts) {
    c.setValue(timestamp, 0);
  }

  for (const auto& var : _vars) {
    increaseCount(timestamp, _solver.value(timestamp, var));
  }

  Int shortage = 0;
  Int excess = 0;

  assert(_counts.size() == _lowerBounds.size());
  for (size_t i = 0; i < _lowerBounds.size(); ++i) {
    if (_lowerBounds.at(i) < 0) {
      continue;
    }
    shortage +=
        std::max(Int(0), _lowerBounds.at(i) - _counts.at(i).value(timestamp));
    excess +=
        std::max(Int(0), _counts.at(i).value(timestamp) - _upperBounds.at(i));
  }

  _shortage.setValue(timestamp, shortage);
  _excess.setValue(timestamp, excess);

  updateValue(timestamp, _violationId, std::max(shortage, excess));
}

void GlobalCardinalityLowUp::notifyInputChanged(Timestamp timestamp,
                                                LocalId localId) {
  assert(localId < _vars.size());
  const Int newValue = _solver.value(timestamp, _vars[localId]);
  const Int committedValue = _solver.committedValue(_vars[localId]);
  if (newValue == committedValue) {
    return;
  }
  const signed char dec = decreaseCount(timestamp, committedValue);
  const signed char inc = increaseCount(timestamp, newValue);
  updateValue(timestamp, _violationId,
              std::max(_shortage.incValue(timestamp, (dec > 0 ? dec : 0) +
                                                         (inc < 0 ? inc : 0)),
                       _excess.incValue(timestamp, (dec < 0 ? dec : 0) +
                                                       (inc > 0 ? inc : 0))));
}

VarViewId GlobalCardinalityLowUp::nextInput(Timestamp timestamp) {
  const auto index = static_cast<size_t>(_state.incValue(timestamp, 1));
  assert(0 <= _state.value(timestamp));
  if (index < _vars.size()) {
    return _vars[index];
  }
  return NULL_ID;
}

void GlobalCardinalityLowUp::notifyCurrentInputChanged(Timestamp timestamp) {
  assert(static_cast<size_t>(_state.value(timestamp)) < _vars.size());
  notifyInputChanged(timestamp, _state.value(timestamp));
}

void GlobalCardinalityLowUp::commit(Timestamp timestamp) {
  Invariant::commit(timestamp);

  _shortage.commitIf(timestamp);
  _excess.commitIf(timestamp);

  for (CommittableInt& CommittableInt : _counts) {
    CommittableInt.commitIf(timestamp);
  }
}
}  // namespace atlantis::propagation
