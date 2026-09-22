#include "atlantis/propagation/invariants/count.hpp"

#include <limits>
#include <optional>
#include <utility>

#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/utils/overflow.hpp"

namespace atlantis::propagation {

namespace {

std::optional<size_t> countIndex(const Int value, const Int offset,
                                 const size_t size) {
  if (value < offset) {
    return std::nullopt;
  }
  const UInt delta = static_cast<UInt>(value) - static_cast<UInt>(offset);
  if (delta >= size) {
    return std::nullopt;
  }
  return static_cast<size_t>(delta);
}

}  // namespace

Count::Count(SolverBase& solver, const VarId output, const VarViewId needle,
             std::vector<VarViewId>&& varArray)
    : Invariant(solver),
      _output(output),
      _needle(needle),
      _vars(std::move(varArray)),
      _offset(0) {}

Count::Count(SolverBase& solver, const VarViewId output, const VarViewId needle,
             std::vector<VarViewId>&& varArray)
    : Count(solver, VarId{output}, needle, std::move(varArray)) {
  assert(output.isVar());
}

inline void Count::increaseCount(const Timestamp ts, const Int value) {
  const auto index = countIndex(value, _offset, _counts.size());
  if (!index.has_value()) {
    return;
  }
  assert(_counts[*index].value(ts) + 1 > 0);
  assert(_counts[*index].value(ts) + 1 <= static_cast<Int>(_vars.size()));
  _counts[*index].incValue(ts, 1);
}

inline void Count::decreaseCount(const Timestamp ts, const Int value) {
  const auto index = countIndex(value, _offset, _counts.size());
  if (!index.has_value()) {
    return;
  }
  assert(_counts[*index].value(ts) - 1 >= 0);
  assert(_counts[*index].value(ts) - 1 < static_cast<Int>(_vars.size()));
  _counts[*index].incValue(ts, -1);
}

inline signed char Count::count(const Timestamp ts, const Int value) const {
  const auto index = countIndex(value, _offset, _counts.size());
  if (!index.has_value()) {
    return 0;
  }
  assert(_counts.at(*index).value(ts) >= 0);
  return static_cast<signed char>(_counts[*index].value(ts));
}

void Count::registerVars() {
  assert(_id != NULL_ID);
  for (size_t i = 0; i < _vars.size(); ++i) {
    _solver.registerInvariantInput(_id, _vars[i], i, false);
  }
  _solver.registerInvariantInput(_id, _needle, _vars.size(), false);
  registerDefinedVar(_output);
}

void Count::updateBounds(const bool widenOnly) {
  _solver.updateBounds(_output, 0, static_cast<Int>(_vars.size()), widenOnly);
}

void Count::close(const Timestamp ts) {
  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::min();

  for (const auto& var : _vars) {
    lb = std::min(lb, _solver.lowerBound(var));
    ub = std::max(ub, _solver.upperBound(var));
  }
  assert(ub >= lb);
  lb = std::max(lb, _solver.lowerBound(_needle));
  ub = std::max(ub, _solver.upperBound(_needle));

  _counts.resize(overflow::saturatingIntervalSize(lb, ub),
                 CommittableInt(ts, 0));
  _offset = lb;
}

void Count::recompute(const Timestamp ts) {
  for (CommittableInt& c : _counts) {
    c.setValue(ts, 0);
  }

  updateValue(ts, _output, 0);

  for (const auto& var : _vars) {
    increaseCount(ts, _solver.value(ts, var));
  }
  updateValue(ts, _output, count(ts, _solver.value(ts, _needle)));
}

void Count::notifyInputChanged(const Timestamp ts, const LocalId id) {
  if (id == _vars.size()) {
    updateValue(ts, _output, count(ts, _solver.value(ts, _needle)));
    return;
  }
  assert(id < _vars.size());
  const Int newValue = _solver.value(ts, _vars[id]);
  const Int committedValue = _solver.committedValue(_vars[id]);
  if (newValue == committedValue) {
    return;
  }
  decreaseCount(ts, committedValue);
  increaseCount(ts, newValue);
  updateValue(ts, _output, count(ts, _solver.value(ts, _needle)));
}

VarViewId Count::nextInput(const Timestamp ts) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  if (index < _vars.size()) {
    return _vars[index];
  }
  if (index == _vars.size()) {
    return _needle;
  }
  return VAR_VIEW_NULL_ID;
}

void Count::notifyCurrentInputChanged(const Timestamp ts) {
  assert(static_cast<size_t>(_state.value(ts)) <= _vars.size());
  notifyInputChanged(ts, static_cast<size_t>(_state.value(ts)));
}

void Count::commit(const Timestamp ts) {
  Invariant::commit(ts);

  for (CommittableInt& committableInt : _counts) {
    committableInt.commitIf(ts);
  }
}
}  // namespace atlantis::propagation
