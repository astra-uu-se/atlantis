#include "constraints/globalCardinality.hpp"

#include "core/engine.hpp"
#include "variables/committableInt.hpp"
/**
 * @param violationId id for the violationCount
 */
template GlobalCardinality<true>::GlobalCardinality(
    VarId violationId, std::vector<VarId> t_variables,
    const std::vector<Int>& cover, const std::vector<Int>& t_counts);
template GlobalCardinality<false>::GlobalCardinality(
    VarId violationId, std::vector<VarId> t_variables,
    const std::vector<Int>& cover, const std::vector<Int>& t_counts);
template <bool IsClosed>
GlobalCardinality<IsClosed>::GlobalCardinality(VarId violationId,
                                               std::vector<VarId> t_variables,
                                               const std::vector<Int>& cover,
                                               const std::vector<Int>& t_counts)
    : GlobalCardinality(violationId, t_variables, cover, t_counts, t_counts) {}

template GlobalCardinality<true>::GlobalCardinality(
    VarId violationId, std::vector<VarId> t_variables,
    const std::vector<Int>& cover, const std::vector<Int>& lowerBound,
    const std::vector<Int>& upperBound);
template GlobalCardinality<false>::GlobalCardinality(
    VarId violationId, std::vector<VarId> t_variables,
    const std::vector<Int>& cover, const std::vector<Int>& lowerBound,
    const std::vector<Int>& upperBound);
template <bool IsClosed>
GlobalCardinality<IsClosed>::GlobalCardinality(
    VarId violationId, std::vector<VarId> t_variables,
    const std::vector<Int>& cover, const std::vector<Int>& lowerBound,
    const std::vector<Int>& upperBound)
    : Constraint(NULL_ID, violationId),
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

template void GlobalCardinality<true>::init(Timestamp, Engine&);
template void GlobalCardinality<false>::init(Timestamp, Engine&);
template <bool IsClosed>
void GlobalCardinality<IsClosed>::init(Timestamp timestamp, Engine& engine) {
  _counts.assign(_lowerBound.size(), CommittableInt(timestamp, 0));

  assert(!_id.equals(NULL_ID));

  for (size_t i = 0; i < _variables.size(); ++i) {
    engine.registerInvariantInput(_id, _variables[i], LocalId(i));
    _localValues.emplace_back(timestamp, engine.committedValue(_variables[i]));
  }

  registerDefinedVariable(engine, _violationId);
}

template void GlobalCardinality<true>::recompute(Timestamp, Engine&);
template void GlobalCardinality<false>::recompute(Timestamp, Engine&);
template <bool IsClosed>
void GlobalCardinality<IsClosed>::recompute(Timestamp timestamp,
                                            Engine& engine) {
  for (CommittableInt& c : _counts) {
    c.setValue(timestamp, 0);
  }

  for (size_t i = 0; i < _variables.size(); ++i) {
    increaseCount(timestamp, engine.value(timestamp, _variables[i]));
    _localValues.at(i).setValue(timestamp,
                                engine.value(timestamp, _variables[i]));
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

  updateValue(timestamp, engine, _violationId, std::max(excess, shortage));
}

template void GlobalCardinality<true>::notifyInputChanged(Timestamp, Engine&,
                                                          LocalId);
template void GlobalCardinality<false>::notifyInputChanged(Timestamp, Engine&,
                                                           LocalId);
template <bool IsClosed>
void GlobalCardinality<IsClosed>::notifyInputChanged(Timestamp timestamp,
                                                     Engine& engine,
                                                     LocalId localId) {
  assert(0 <= localId && localId < _localValues.size());
  const Int oldValue = _localValues[localId].value(timestamp);
  const Int newValue = engine.value(timestamp, _variables[localId]);
  if (newValue == oldValue) {
    return;
  }
  const signed char dec = decreaseCount(timestamp, oldValue);
  const signed char inc = increaseCount(timestamp, newValue);
  _localValues[localId].setValue(timestamp, newValue);
  updateValue(timestamp, engine, _violationId,
              std::max(_shortage.incValue(timestamp, (dec > 0 ? dec : 0) +
                                                         (inc < 0 ? inc : 0)),
                       _excess.incValue(timestamp, (dec < 0 ? dec : 0) +
                                                       (inc > 0 ? inc : 0))));
}

template VarId GlobalCardinality<true>::nextInput(Timestamp, Engine&);
template VarId GlobalCardinality<false>::nextInput(Timestamp, Engine&);
template <bool IsClosed>
VarId GlobalCardinality<IsClosed>::nextInput(Timestamp timestamp, Engine&) {
  const auto index = static_cast<size_t>(_state.incValue(timestamp, 1));
  assert(0 <= _state.value(timestamp));
  if (index < _variables.size()) {
    return _variables[index];
  }
  return NULL_ID;
}

template void GlobalCardinality<true>::notifyCurrentInputChanged(Timestamp,
                                                                 Engine&);
template void GlobalCardinality<false>::notifyCurrentInputChanged(Timestamp,
                                                                  Engine&);
template <bool IsClosed>
void GlobalCardinality<IsClosed>::notifyCurrentInputChanged(Timestamp timestamp,
                                                            Engine& engine) {
  assert(static_cast<size_t>(_state.value(timestamp)) < _variables.size());
  notifyInputChanged(timestamp, engine, _state.value(timestamp));
}

template void GlobalCardinality<true>::commit(Timestamp, Engine&);
template void GlobalCardinality<false>::commit(Timestamp, Engine&);
template <bool IsClosed>
void GlobalCardinality<IsClosed>::commit(Timestamp timestamp, Engine& engine) {
  Invariant::commit(timestamp, engine);

  _shortage.commitIf(timestamp);
  _excess.commitIf(timestamp);

  for (auto& localValue : _localValues) {
    localValue.commitIf(timestamp);
  }

  for (CommittableInt& CommittableInt : _counts) {
    CommittableInt.commitIf(timestamp);
  }
}
