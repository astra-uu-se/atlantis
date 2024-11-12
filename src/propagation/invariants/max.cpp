#include "atlantis/propagation/invariants/max.hpp"

#include <limits>
#include <numeric>
#include <utility>
#include <vector>

namespace atlantis::propagation {

Max::Max(SolverBase& solver, VarId output, std::vector<VarViewId>&& varArray)
    : Invariant(solver),
      _output(output),
      _varArray(std::move(varArray)),
      _linkedList(_varArray.size(),
                  std ::pair<CommittableInt, CommittableInt>{
                      CommittableInt{NULL_TIMESTAMP, -1},
                      CommittableInt{NULL_TIMESTAMP, -1}}),
      _listHead(NULL_TIMESTAMP, -1),
      _updatedMax(NULL_TIMESTAMP, std::numeric_limits<Int>::min()),
      _limit(std::numeric_limits<Int>::max()) {}

Max::Max(SolverBase& solver, VarViewId output,
         std::vector<VarViewId>&& varArray)
    : Max(solver, VarId(output), std::move(varArray)) {
  assert(output.isVar());
}

void Max::registerVars() {
  assert(_id != NULL_ID);
  for (size_t i = 0; i < _varArray.size(); ++i) {
    _solver.registerInvariantInput(_id, _varArray[i], i, false);
  }
  registerDefinedVar(_output);
}

void Max::updateBounds(bool widenOnly) {
  Int lb = std::numeric_limits<Int>::min();
  Int ub = std::numeric_limits<Int>::min();
  for (const VarViewId& input : _varArray) {
    lb = std::max(lb, _solver.lowerBound(input));
    ub = std::max(ub, _solver.upperBound(input));
  }
  _solver.updateBounds(_output, std::min(_limit, lb), std::min(_limit, ub),
                       widenOnly);
}

void Max::close(Timestamp ts) {
  // sort indices:
  std::vector<size_t> idx(_varArray.size());
  std::iota(idx.begin(), idx.end(), 0);

  std::stable_sort(idx.begin(), idx.end(), [&](size_t i1, size_t i2) {
    return _solver.value(ts, _varArray[i1]) > _solver.value(ts, _varArray[i2]);
  });

  for (size_t i = 0; i < _varArray.size(); ++i) {
    _linkedList[idx[i]].first.setValue(ts, i == 0 ? -1 : idx[i - 1]);
    _linkedList[idx[i]].second.setValue(
        ts, (i + 1 == _varArray.size()) ? -1 : idx[i + 1]);
  }

  _listHead.setValue(ts, idx.front());
  _updatedMax.setValue(ts, std::numeric_limits<Int>::min());
}

void Max::recompute(Timestamp ts) {
  close(ts);
  assert(0 <= _listHead.value(ts) &&
         _listHead.value(ts) < static_cast<Int>(_varArray.size()));
  assert(_varArray.at(_listHead.value(ts)) ==
         *std::min_element(_varArray.begin(), _varArray.end(),
                           [&](const VarViewId& v1, const VarViewId& v2) {
                             return _solver.value(ts, v1) >
                                    _solver.value(ts, v2);
                           }));

  updateValue(ts, _output, _solver.value(ts, _varArray[_listHead.value(ts)]));
}

void Max::notifyInputChanged(Timestamp ts, LocalId id) {
  assert(id < _varArray.size());

  const Int prev = _linkedList[id].first.value(ts);
  const Int next = _linkedList[id].second.value(ts);

  // unlink id
  _linkedList[id].first.setValue(ts, -1);
  _linkedList[id].second.setValue(ts, -1);

  // update linked list
  if (0 <= prev) {
    assert(_linkedList.at(prev).second.value(ts) == static_cast<Int>(id));
    _linkedList[prev].second.setValue(ts, next);
  } else {
    // This is the head of the list
    assert(_listHead.value(ts) == static_cast<Int>(id));
    _listHead.setValue(ts, next);
  }
  if (0 <= next) {
    assert(_linkedList.at(next).first.value(ts) == static_cast<Int>(id));
    _linkedList[next].first.setValue(ts, prev);
  }

  _updatedMax.setValue(
      ts, std::max(_updatedMax.value(ts), _solver.value(ts, _varArray[id])));

  if (0 <= _listHead.value(ts)) {
    updateValue(ts, _output,
                std::max(_updatedMax.value(ts),
                         _solver.value(ts, _varArray[_listHead.value(ts)])));
  } else {
    updateValue(ts, _output, _updatedMax.value(ts));
  }
}

VarViewId Max::nextInput(Timestamp ts) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  assert(0 <= _state.value(ts));
  if (index == 0 ||
      (index < _varArray.size() && _solver.value(ts, _varArray[index - 1]) !=
                                       _solver.upperBound(_output))) {
    return _varArray[index];
  } else {
    return NULL_ID;  // Done
  }
}

void Max::notifyCurrentInputChanged(Timestamp ts) {
  notifyInputChanged(ts, _state.value(ts));
}

void Max::commit(Timestamp ts) {
  Invariant::commit(ts);
  close(ts);
  for (size_t i = 0; i < _varArray.size(); ++i) {
    _linkedList[i].first.commitIf(ts);
    _linkedList[i].second.commitIf(ts);
  }
  _listHead.commitIf(ts);
  // _updatedMax should NOT be committed.
}

}  // namespace atlantis::propagation
