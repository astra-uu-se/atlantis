#include "propagation/violationInvariants/globalCardinalityLowUp.hpp"

namespace atlantis::propagation {

/**
 * @param violationId id for the violationCount
 */

GlobalCardinalityLowUp::GlobalCardinalityLowUp(SolverBase& solver,
                                               VarId violationId,
                                               std::vector<VarId>&& t_vars,
                                               const std::vector<Int>& cover,
                                               const std::vector<Int>& bounds)
    : GlobalCardinalityLowUp(solver, violationId, std::move(t_vars), cover,
                             bounds, bounds) {}

GlobalCardinalityLowUp::GlobalCardinalityLowUp(
    SolverBase& solver, VarId violationId, std::vector<VarId>&& t_vars,
    const std::vector<Int>& cover, const std::vector<Int>& lowerBound,
    const std::vector<Int>& upperBound)
    : ViolationInvariant(solver, violationId),
      _vars(std::move(t_vars)),
      _lowerBounds(),
      _upperBounds(),
      _committedValues(_vars.size(), 0),
      _shortage(NULL_TIMESTAMP, 0),
      _excess(NULL_TIMESTAMP, 0),
      _counts(),
      _offset(0) {
  _modifiedVars.reserve(_vars.size());
  assert(lowerBound.size() == upperBound.size() &&
         lowerBound.size() == cover.size());

  const auto [lb, ub] = std::minmax_element(cover.begin(), cover.end());

  // a bound of -1 means that the count of a value is not restricted:
  _lowerBounds.assign(static_cast<Int>(*ub - *lb + 3), -1);
  _upperBounds.assign(static_cast<Int>(*ub - *lb + 3), -1);
  _offset = *lb - 1;

  for (size_t i = 0; i < cover.size(); ++i) {
    assert(lowerBound[i] >= 0);
    assert(lowerBound[i] <= upperBound[i]);
    _lowerBounds[cover[i] - _offset] = lowerBound[i];
    _upperBounds[cover[i] - _offset] = upperBound[i];
  }
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
  assert(localId < _committedValues.size());
  const Int newValue = _solver.value(timestamp, _vars[localId]);
  if (newValue == _committedValues[localId]) {
    return;
  }
  const signed char dec = decreaseCount(timestamp, _committedValues[localId]);
  const signed char inc = increaseCount(timestamp, newValue);
  updateValue(timestamp, _violationId,
              std::max(_shortage.incValue(timestamp, (dec > 0 ? dec : 0) +
                                                         (inc < 0 ? inc : 0)),
                       _excess.incValue(timestamp, (dec < 0 ? dec : 0) +
                                                       (inc > 0 ? inc : 0))));
}

VarId GlobalCardinalityLowUp::nextInput(Timestamp timestamp) {
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

  for (size_t i = 0; i < _committedValues.size(); ++i) {
    _committedValues[i] = _solver.committedValue(_vars[i]);
  }

  for (CommittableInt& CommittableInt : _counts) {
    CommittableInt.commitIf(timestamp);
  }
}
}  // namespace atlantis::propagation