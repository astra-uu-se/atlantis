#include "constraints/globalCardinalityConst.hpp"

#include "core/engine.hpp"
/**
 * @param violationId id for the violationCount
 */
template GlobalCardinalityConst<true>::GlobalCardinalityConst(
    Engine& engine, VarId violationId, std::vector<VarId> t_variables,
    const std::vector<Int>& cover, const std::vector<Int>& t_counts);
template GlobalCardinalityConst<false>::GlobalCardinalityConst(
    Engine& engine, VarId violationId, std::vector<VarId> t_variables,
    const std::vector<Int>& cover, const std::vector<Int>& t_counts);
template <bool IsClosed>
GlobalCardinalityConst<IsClosed>::GlobalCardinalityConst(
    Engine& engine, VarId violationId, std::vector<VarId> t_variables,
    const std::vector<Int>& cover, const std::vector<Int>& t_counts)
    : GlobalCardinalityConst(engine, violationId, t_variables, cover, t_counts,
                             t_counts) {}

template GlobalCardinalityConst<true>::GlobalCardinalityConst(
    Engine& engine, VarId violationId, std::vector<VarId> t_variables,
    const std::vector<Int>& cover, const std::vector<Int>& lowerBound,
    const std::vector<Int>& upperBound);
template GlobalCardinalityConst<false>::GlobalCardinalityConst(
    Engine& engine, VarId violationId, std::vector<VarId> t_variables,
    const std::vector<Int>& cover, const std::vector<Int>& lowerBound,
    const std::vector<Int>& upperBound);
template <bool IsClosed>
GlobalCardinalityConst<IsClosed>::GlobalCardinalityConst(
    Engine& engine, VarId violationId, std::vector<VarId> t_variables,
    const std::vector<Int>& cover, const std::vector<Int>& lowerBound,
    const std::vector<Int>& upperBound)
    : Constraint(engine, violationId),
      _variables(std::move(t_variables)),
      _lowerBound(),
      _upperBound(),
      _localValues(),
      _shortage(NULL_TIMESTAMP, 0),
      _excess(NULL_TIMESTAMP, 0),
      _counts(),
      _offset(0) {
  _modifiedVars.reserve(_variables.size());
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
  for (size_t i = 0; i < _variables.size(); ++i) {
    _engine.registerInvariantInput(_id, _variables[i], LocalId(i));
  }
  registerDefinedVariable(_violationId);
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
    maxExcess = std::max(maxExcess, static_cast<Int>(_variables.size()) - ub);
  }
  Int maxViol = std::max(maxShortage, maxExcess);
  if constexpr (IsClosed) {
    maxViol =
        std::max(maxViol, maxShortage + static_cast<Int>(_variables.size()));
  }
  _engine.updateBounds(_violationId, 0, maxViol, widenOnly);
}

template void GlobalCardinalityConst<true>::close(Timestamp);
template void GlobalCardinalityConst<false>::close(Timestamp);
template <bool IsClosed>
void GlobalCardinalityConst<IsClosed>::close(Timestamp timestamp) {
  _counts.resize(_lowerBound.size(), CommittableInt(timestamp, 0));
  _localValues.clear();
  for (size_t i = 0; i < _variables.size(); ++i) {
    _localValues.emplace_back(timestamp, _engine.committedValue(_variables[i]));
  }
}

template void GlobalCardinalityConst<true>::recompute(Timestamp);
template void GlobalCardinalityConst<false>::recompute(Timestamp);
template <bool IsClosed>
void GlobalCardinalityConst<IsClosed>::recompute(Timestamp timestamp) {
  for (CommittableInt& c : _counts) {
    c.setValue(timestamp, 0);
  }

  for (size_t i = 0; i < _variables.size(); ++i) {
    increaseCount(timestamp, _engine.value(timestamp, _variables[i]));
    _localValues.at(i).setValue(timestamp,
                                _engine.value(timestamp, _variables[i]));
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
  assert(localId < _localValues.size());
  const Int oldValue = _localValues[localId].value(timestamp);
  const Int newValue = _engine.value(timestamp, _variables[localId]);
  if (newValue == oldValue) {
    return;
  }
  const signed char dec = decreaseCount(timestamp, oldValue);
  const signed char inc = increaseCount(timestamp, newValue);
  _localValues[localId].setValue(timestamp, newValue);
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
  if (index < _variables.size()) {
    return _variables[index];
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
  assert(static_cast<size_t>(_state.value(timestamp)) < _variables.size());
  notifyInputChanged(timestamp, _state.value(timestamp));
}

template void GlobalCardinalityConst<true>::commit(Timestamp);
template void GlobalCardinalityConst<false>::commit(Timestamp);
template <bool IsClosed>
void GlobalCardinalityConst<IsClosed>::commit(Timestamp timestamp) {
  Invariant::commit(timestamp);

  _shortage.commitIf(timestamp);
  _excess.commitIf(timestamp);

  for (auto& localValue : _localValues) {
    localValue.commitIf(timestamp);
  }

  for (CommittableInt& CommittableInt : _counts) {
    CommittableInt.commitIf(timestamp);
  }
}
