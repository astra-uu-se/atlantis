#include "propagation/violationInvariants/globalCardinalityClosed.hpp"

namespace atlantis::propagation {

inline bool all_in_range(Int start, Int stop,
                         std::function<bool(Int)> predicate) {
  std::vector<Int> vec(stop - start);
  for (Int i = 0; i < stop - start; ++i) {
    vec.at(i) = start + i;
  }
  return std::all_of(vec.begin(), vec.end(), predicate);
}

/**
 * @param violationId id for the violationCount
 */
GlobalCardinalityClosed::GlobalCardinalityClosed(SolverBase& solver,
                                                 VarId violationId,
                                                 std::vector<VarId> outputs,
                                                 std::vector<VarId> inputs,
                                                 std::vector<Int> cover)
    : ViolationInvariant(solver, violationId),
      _outputs(std::move(outputs)),
      _inputs(std::move(inputs)),
      _cover(std::move(cover)),
      _coverVarIndex(),
      _committedValues(_inputs.size(), 0),
      _counts(),
      _offset(0) {
  _modifiedVars.reserve(_inputs.size());
  assert(_cover.size() == _outputs.size());
  assert(all_in_range(0, _cover.size(), [&](const size_t i) {
    return all_in_range(i + 1, _cover.size(), [&](const size_t j) {
      return _cover.at(i) != _cover.at(j) && _outputs.at(i) != _outputs.at(j);
    });
  }));
}

void GlobalCardinalityClosed::registerVars() {
  assert(!_id.equals(NULL_ID));
  for (size_t i = 0; i < _inputs.size(); ++i) {
    _solver.registerInvariantInput(_id, _inputs[i], LocalId(i));
  }
  registerDefinedVar(_violationId);
  for (const VarId output : _outputs) {
    registerDefinedVar(output);
  }
}

void GlobalCardinalityClosed::updateBounds(bool widenOnly) {
  _solver.updateBounds(_violationId, 0, _inputs.size(), widenOnly);
  for (const VarId output : _outputs) {
    _solver.updateBounds(output, 0, _inputs.size(), widenOnly);
  }
}

void GlobalCardinalityClosed::close(Timestamp timestamp) {
  const auto [lb, ub] = std::minmax_element(_cover.begin(), _cover.end());
  _offset = *lb;
  _coverVarIndex.resize(*ub - *lb + 1, -1);
  for (size_t i = 0; i < _cover.size(); ++i) {
    assert(0 <= _cover[i] - _offset);
    assert(_cover[i] - _offset < static_cast<Int>(_coverVarIndex.size()));
    _coverVarIndex[_cover[i] - _offset] = i;
  }
  _counts.resize(_outputs.size(), CommittableInt(timestamp, 0));
}

void GlobalCardinalityClosed::recompute(Timestamp timestamp) {
  for (CommittableInt& c : _counts) {
    c.setValue(timestamp, 0);
  }

  Int excess = 0;
  for (size_t i = 0; i < _inputs.size(); ++i) {
    excess += increaseCount(timestamp, _solver.value(timestamp, _inputs[i]));
  }

  updateValue(timestamp, _violationId, excess);
  for (size_t i = 0; i < _outputs.size(); ++i) {
    assert(0 <= _cover[i] - _offset &&
           _cover[i] - _offset < static_cast<Int>(_coverVarIndex.size()));
    updateValue(timestamp, _outputs[i], _counts[i].value(timestamp));
  }
}

void GlobalCardinalityClosed::notifyInputChanged(Timestamp timestamp,
                                                 LocalId localId) {
  assert(localId < _committedValues.size());
  const Int newValue = _solver.value(timestamp, _inputs[localId]);
  if (newValue == _committedValues[localId]) {
    return;
  }
  const Int dec = decreaseCount(timestamp, _committedValues[localId]);
  const Int inc = increaseCount(timestamp, newValue);
  incValue(timestamp, _violationId, inc - dec);
  updateOutput(timestamp, _committedValues[localId]);
  updateOutput(timestamp, newValue);
}

VarId GlobalCardinalityClosed::nextInput(Timestamp timestamp) {
  const auto index = static_cast<size_t>(_state.incValue(timestamp, 1));
  assert(0 <= _state.value(timestamp));
  if (index < _inputs.size()) {
    return _inputs[index];
  }
  return NULL_ID;
}

void GlobalCardinalityClosed::notifyCurrentInputChanged(Timestamp timestamp) {
  assert(static_cast<size_t>(_state.value(timestamp)) < _inputs.size());
  notifyInputChanged(timestamp, _state.value(timestamp));
}

void GlobalCardinalityClosed::commit(Timestamp timestamp) {
  Invariant::commit(timestamp);

  for (size_t i = 0; i < _committedValues.size(); ++i) {
    _committedValues[i] = _solver.committedValue(_inputs[i]);
  }

  for (CommittableInt& CommittableInt : _counts) {
    CommittableInt.commitIf(timestamp);
  }
}
}  // namespace atlantis::propagation