#include "atlantis/propagation/violationInvariants/allDifferentExcept.hpp"

#include <algorithm>
#include <cassert>

namespace atlantis::propagation {

/**
 * @param violationId id for the violationCount
 */
AllDifferentExcept::AllDifferentExcept(SolverBase& solver, VarId violationId,
                                       std::vector<VarViewId>&& vars,
                                       const std::vector<Int>& ignored)
    : AllDifferent(solver, violationId, std::move(vars)) {
  const auto [lb, ub] =
      std::minmax_element(std::begin(ignored), std::end(ignored));
  assert(*lb <= *ub);
  _ignored.resize(static_cast<size_t>((*ub) - (*lb) + 1), false);
  _ignoredOffset = *lb;
  for (const Int val : ignored) {
    _ignored[val - _ignoredOffset] = true;
  }
}

AllDifferentExcept::AllDifferentExcept(SolverBase& solver,
                                       VarViewId violationId,
                                       std::vector<VarViewId>&& vars,
                                       const std::vector<Int>& ignored)
    : AllDifferentExcept(solver, VarId(violationId), std::move(vars), ignored) {
  assert(violationId.isVar());
}

void AllDifferentExcept::recompute(Timestamp ts) {
  for (CommittableInt& c : _counts) {
    c.setValue(ts, 0);
  }

  Int violInc = 0;
  for (const auto& var : _vars) {
    const Int val = _solver.value(ts, var);
    if (!isIgnored(val)) {
      violInc += increaseCount(ts, val);
    }
  }
  updateValue(ts, _violationId, violInc);
}

void AllDifferentExcept::notifyInputChanged(Timestamp ts, LocalId id) {
  assert(id < _vars.size());
  const Int newValue = _solver.value(ts, _vars[id]);
  const Int committedValue = _solver.committedValue(_vars[id]);
  if (newValue == committedValue) {
    return;
  }
  incValue(ts, _violationId,
           (isIgnored(committedValue) ? 0 : decreaseCount(ts, committedValue)) +
               (isIgnored(newValue) ? 0 : increaseCount(ts, newValue)));
}
}  // namespace atlantis::propagation
