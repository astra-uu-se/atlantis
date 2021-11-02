#include "constraints/globalCardinality.hpp"

#include "core/engine.hpp"
#include "variables/savedInt.hpp"
/**
 * @param violationId id for the violationCount
 */
template GlobalCardinality<true>::GlobalCardinality(
    VarId violationId, std::vector<VarId> t_variables, std::vector<Int> cover,
    std::vector<Int> t_counts);
template GlobalCardinality<false>::GlobalCardinality(
    VarId violationId, std::vector<VarId> t_variables, std::vector<Int> cover,
    std::vector<Int> t_counts);
template <bool IsClosed>
GlobalCardinality<IsClosed>::GlobalCardinality(VarId violationId,
                                               std::vector<VarId> t_variables,
                                               std::vector<Int> cover,
                                               std::vector<Int> t_counts)
    : GlobalCardinality(violationId, t_variables, cover, t_counts, t_counts) {}

template GlobalCardinality<true>::GlobalCardinality(
    VarId violationId, std::vector<VarId> t_variables, std::vector<Int> cover,
    std::vector<Int> lowerBound, std::vector<Int> upperBound);
template GlobalCardinality<false>::GlobalCardinality(
    VarId violationId, std::vector<VarId> t_variables, std::vector<Int> cover,
    std::vector<Int> lowerBound, std::vector<Int> upperBound);
template <bool IsClosed>
GlobalCardinality<IsClosed>::GlobalCardinality(VarId violationId,
                                               std::vector<VarId> t_variables,
                                               std::vector<Int> cover,
                                               std::vector<Int> lowerBound,
                                               std::vector<Int> upperBound)
    : Constraint(NULL_ID, violationId),
      _variables(std::move(t_variables)),
      _cover(std::move(cover)),
      _lowerBound(),
      _upperBound(),
      _localValues(),
      _excess(NULL_TIMESTAMP, 0),
      _shortage(NULL_TIMESTAMP, 0),
      _counts(),
      _offset(0) {
  _modifiedVars.reserve(_variables.size());
  assert(lowerBound.size() == upperBound.size() &&
         lowerBound.size() == _cover.size());

  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::min();

  for (Int val : _cover) {
    lb = std::min(lb, val);
    ub = std::max(ub, val);
  }

  assert(ub >= lb);

  if constexpr (IsClosed) {
    _lowerBound.resize(static_cast<unsigned long>(ub - lb + 3), 0);
    _upperBound.resize(static_cast<unsigned long>(ub - lb + 3), 0);
  } else {
    _lowerBound.resize(static_cast<unsigned long>(ub - lb + 3), -1);
    _upperBound.resize(static_cast<unsigned long>(ub - lb + 3), -1);
  }
  _offset = lb - 1;

  for (size_t i = 0; i < _cover.size(); ++i) {
    assert(lowerBound[i] >= 0);
    assert(lowerBound[i] <= upperBound[i]);
    _lowerBound[_cover[i] - _offset] = lowerBound[i];
    _upperBound[_cover[i] - _offset] = upperBound[i];
  }
}

template void GlobalCardinality<true>::init(Timestamp, Engine&);
template void GlobalCardinality<false>::init(Timestamp, Engine&);
template <bool IsClosed>
void GlobalCardinality<IsClosed>::init(Timestamp ts, Engine& e) {
  _counts.resize(_lowerBound.size(), SavedInt(ts, 0));

  assert(!_id.equals(NULL_ID));

  for (size_t i = 0; i < _variables.size(); ++i) {
    e.registerInvariantInput(_id, _variables[i], LocalId(i));
    _localValues.emplace_back(ts, e.getCommittedValue(_variables[i]));
  }

  registerDefinedVariable(e, _violationId);
}

template void GlobalCardinality<true>::recompute(Timestamp, Engine&);
template void GlobalCardinality<false>::recompute(Timestamp, Engine&);
template <bool IsClosed>
void GlobalCardinality<IsClosed>::recompute(Timestamp ts, Engine& e) {
  for (SavedInt& c : _counts) {
    c.setValue(ts, 0);
  }

  for (size_t i = 0; i < _variables.size(); ++i) {
    increaseCount(ts, e.getValue(ts, _variables[i]));
    _localValues[i].setValue(ts, e.getValue(ts, _variables[i]));
  }

  Int excess = 0;
  Int shortage = 0;
  for (Int val : _cover) {
    if (_counts.at(val - _offset).getValue(ts) <
        _lowerBound.at(val - _offset)) {
      shortage += _lowerBound.at(val - _offset) -
                  _counts.at(val - _offset).getValue(ts);
    } else if (_upperBound.at(val - _offset) <
               _counts.at(val - _offset).getValue(ts)) {
      excess += _counts.at(val - _offset).getValue(ts) -
                _upperBound.at(val - _offset);
    }
  }

  _excess.setValue(ts, excess);
  _shortage.setValue(ts, shortage);

  updateValue(ts, e, _violationId, std::max(excess, shortage));
}

template void GlobalCardinality<true>::notifyIntChanged(Timestamp, Engine&,
                                                        LocalId);
template void GlobalCardinality<false>::notifyIntChanged(Timestamp, Engine&,
                                                         LocalId);
template <bool IsClosed>
void GlobalCardinality<IsClosed>::notifyIntChanged(Timestamp ts, Engine& e,
                                                   LocalId id) {
  Int oldValue = _localValues.at(id).getValue(ts);
  Int newValue = e.getValue(ts, _variables[id]);
  if (newValue == oldValue) {
    return;
  }
  signed char dec = decreaseCount(ts, oldValue);
  signed char inc = increaseCount(ts, newValue);
  _localValues.at(id).setValue(ts, newValue);
  _excess.incValue(ts, (dec < 0 ? dec : 0) + (inc > 0 ? inc : 0));
  _shortage.incValue(ts, (dec > 0 ? dec : 0) + (inc < 0 ? inc : 0));

  updateValue(ts, e, _violationId,
              std::max(_excess.getValue(ts), _shortage.getValue(ts)));
}

template VarId GlobalCardinality<true>::getNextInput(Timestamp, Engine&);
template VarId GlobalCardinality<false>::getNextInput(Timestamp, Engine&);
template <bool IsClosed>
VarId GlobalCardinality<IsClosed>::getNextInput(Timestamp ts, Engine&) {
  _state.incValue(ts, 1);

  auto index = static_cast<size_t>(_state.getValue(ts));
  if (index < _variables.size()) {
    return _variables.at(index);
  }
  return NULL_ID;
}

template void GlobalCardinality<true>::notifyCurrentInputChanged(Timestamp,
                                                                 Engine&);
template void GlobalCardinality<false>::notifyCurrentInputChanged(Timestamp,
                                                                  Engine&);
template <bool IsClosed>
void GlobalCardinality<IsClosed>::notifyCurrentInputChanged(Timestamp ts,
                                                            Engine& e) {
  auto id = static_cast<size_t>(_state.getValue(ts));
  assert(id < _variables.size());
  notifyIntChanged(ts, e, id);
}

template void GlobalCardinality<true>::commit(Timestamp, Engine&);
template void GlobalCardinality<false>::commit(Timestamp, Engine&);
template <bool IsClosed>
void GlobalCardinality<IsClosed>::commit(Timestamp ts, Engine& e) {
  Invariant::commit(ts, e);

  _excess.commitIf(ts);
  _shortage.commitIf(ts);

  for (auto& localValue : _localValues) {
    localValue.commitIf(ts);
  }

  for (SavedInt& savedInt : _counts) {
    savedInt.commitIf(ts);
  }
}
