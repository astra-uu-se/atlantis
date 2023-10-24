#include "propagation/constraints/globalCardinalityConst.hpp"

namespace atlantis::propagation {

/**
 * @param violationId id for the violationCount
 */
template GlobalCardinalityConst<true>::GlobalCardinalityConst(
    SolverBase& solver, VarId violationId, std::vector<VarId> t_vars,
    const std::vector<Int>& cover, const std::vector<Int>& t_counts);
template GlobalCardinalityConst<false>::GlobalCardinalityConst(
    SolverBase& solver, VarId violationId, std::vector<VarId> t_vars,
    const std::vector<Int>& cover, const std::vector<Int>& t_counts);
template <bool IsClosed>
GlobalCardinalityConst<IsClosed>::GlobalCardinalityConst(
    SolverBase& solver, VarId violationId, std::vector<VarId> t_vars,
    const std::vector<Int>& cover, const std::vector<Int>& t_counts)
    : GlobalCardinalityConst(solver, violationId, t_vars, cover, t_counts,
                             t_counts) {}

template GlobalCardinalityConst<true>::GlobalCardinalityConst(
    SolverBase& solver, VarId violationId, std::vector<VarId> t_vars,
    const std::vector<Int>& cover, const std::vector<Int>& lowerBound,
    const std::vector<Int>& upperBound);
template GlobalCardinalityConst<false>::GlobalCardinalityConst(
    SolverBase& solver, VarId violationId, std::vector<VarId> t_vars,
    const std::vector<Int>& cover, const std::vector<Int>& lowerBound,
    const std::vector<Int>& upperBound);
template <bool IsClosed>
GlobalCardinalityConst<IsClosed>::GlobalCardinalityConst(
    SolverBase& solver, VarId violationId, std::vector<VarId> t_vars,
    const std::vector<Int>& cover, const std::vector<Int>& lowerBound,
    const std::vector<Int>& upperBound)
    : Constraint(solver, violationId),
      _vars(std::move(t_vars)),
      _lowerBound(),
      _upperBound(),
      _committedValues(_vars.size(), 0),
      _shortage(NULL_TIMESTAMP, 0),
      _excess(NULL_TIMESTAMP, 0),
      _counts(),
      _offset(0) {
  _modifiedVars.reserve(_vars.size());
  assert(lowerBound.size() == upperBound.size() &&
         lowerBound.size() == cover.size());

  const auto [lb, ub] = std::minmax_element(cover.begin(), cover.end());

  if constexpr (IsClosed) {
    _lowerBound.assign(static_cast<Int>(*ub - *lb + 3), 0);
    _upperBound.assign(static_cast<Int>(*ub - *lb + 3), 0);
  } else {
    // a bound of -1 means that the count of a value is not restricted:
    _lowerBound.assign(static_cast<Int>(*ub - *lb + 3), -1);
    _upperBound.assign(static_cast<Int>(*ub - *lb + 3), -1);
  }
  _offset = *lb - 1;

  for (size_t i = 0; i < cover.size(); ++i) {
    assert(lowerBound[i] >= 0);
    assert(lowerBound[i] <= upperBound[i]);
    _lowerBound[cover[i] - _offset] = lowerBound[i];
    _upperBound[cover[i] - _offset] = upperBound[i];
  }
}

template void GlobalCardinalityConst<true>::registerVars();
template void GlobalCardinalityConst<false>::registerVars();
template <bool IsClosed>
void GlobalCardinalityConst<IsClosed>::registerVars() {
  assert(!_id.equals(NULL_ID));
  for (size_t i = 0; i < _vars.size(); ++i) {
    _solver.registerInvariantInput(_id, _vars[i], LocalId(i));
  }
  registerDefinedVar(_violationId);
}

template void GlobalCardinalityConst<true>::updateBounds(bool widenOnly);
template void GlobalCardinalityConst<false>::updateBounds(bool widenOnly);
template <bool IsClosed>
void GlobalCardinalityConst<IsClosed>::updateBounds(bool widenOnly) {
  Int maxShortage = 0;
  for (const Int lb : _lowerBound) {
    maxShortage += lb;
  }
  Int maxExcess = 0;
  for (const Int ub : _upperBound) {
    maxExcess = std::max(maxExcess, static_cast<Int>(_vars.size()) - ub);
  }
  Int maxViol = std::max(maxShortage, maxExcess);
  if constexpr (IsClosed) {
    maxViol = std::max(maxViol, maxShortage + static_cast<Int>(_vars.size()));
  }
  _solver.updateBounds(_violationId, 0, maxViol, widenOnly);
}

template void GlobalCardinalityConst<true>::close(Timestamp);
template void GlobalCardinalityConst<false>::close(Timestamp);
template <bool IsClosed>
void GlobalCardinalityConst<IsClosed>::close(Timestamp timestamp) {
  _counts.resize(_lowerBound.size(), CommittableInt(timestamp, 0));
}

template void GlobalCardinalityConst<true>::recompute(Timestamp);
template void GlobalCardinalityConst<false>::recompute(Timestamp);
template <bool IsClosed>
void GlobalCardinalityConst<IsClosed>::recompute(Timestamp timestamp) {
  for (CommittableInt& c : _counts) {
    c.setValue(timestamp, 0);
  }

  for (size_t i = 0; i < _vars.size(); ++i) {
    increaseCount(timestamp, _solver.value(timestamp, _vars[i]));
  }

  Int shortage = 0;
  Int excess = 0;

  assert(_counts.size() == _lowerBound.size());
  for (size_t i = 0; i < _lowerBound.size(); ++i) {
    if (_lowerBound.at(i) < 0) {
      continue;
    }
    shortage +=
        std::max(Int(0), _lowerBound.at(i) - _counts.at(i).value(timestamp));
    excess +=
        std::max(Int(0), _counts.at(i).value(timestamp) - _upperBound.at(i));
  }

  _shortage.setValue(timestamp, shortage);
  _excess.setValue(timestamp, excess);

  updateValue(timestamp, _violationId, std::max(excess, shortage));
}

template void GlobalCardinalityConst<true>::notifyInputChanged(Timestamp,
                                                               LocalId);
template void GlobalCardinalityConst<false>::notifyInputChanged(Timestamp,
                                                                LocalId);
template <bool IsClosed>
void GlobalCardinalityConst<IsClosed>::notifyInputChanged(Timestamp timestamp,
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

template VarId GlobalCardinalityConst<true>::nextInput(Timestamp);
template VarId GlobalCardinalityConst<false>::nextInput(Timestamp);
template <bool IsClosed>
VarId GlobalCardinalityConst<IsClosed>::nextInput(Timestamp timestamp) {
  const auto index = static_cast<size_t>(_state.incValue(timestamp, 1));
  assert(0 <= _state.value(timestamp));
  if (index < _vars.size()) {
    return _vars[index];
  }
  return NULL_ID;
}

template void GlobalCardinalityConst<true>::notifyCurrentInputChanged(
    Timestamp);
template void GlobalCardinalityConst<false>::notifyCurrentInputChanged(
    Timestamp);
template <bool IsClosed>
void GlobalCardinalityConst<IsClosed>::notifyCurrentInputChanged(
    Timestamp timestamp) {
  assert(static_cast<size_t>(_state.value(timestamp)) < _vars.size());
  notifyInputChanged(timestamp, _state.value(timestamp));
}

template void GlobalCardinalityConst<true>::commit(Timestamp);
template void GlobalCardinalityConst<false>::commit(Timestamp);
template <bool IsClosed>
void GlobalCardinalityConst<IsClosed>::commit(Timestamp timestamp) {
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