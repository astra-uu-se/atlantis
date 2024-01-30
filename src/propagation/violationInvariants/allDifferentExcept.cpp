#include "propagation/violationInvariants/allDifferentExcept.hpp"

namespace atlantis::propagation {

/**
 * @param violationId id for the violationCount
 */
AllDifferentExcept::AllDifferentExcept(SolverBase& solver, VarId violationId,
                                       std::vector<VarId>&& vars,
                                       const std::vector<Int>& ignored)
    : AllDifferent(solver, violationId, std::move(vars)) {
  _modifiedVars.reserve(_vars.size());
  const auto [lb, ub] =
      std::minmax_element(std::begin(ignored), std::end(ignored));
  assert(*lb <= *ub);
  _ignored.resize(static_cast<size_t>((*ub) - (*lb) + 1), false);
  _ignoredOffset = *lb;
  for (const Int val : ignored) {
    _ignored[val - _ignoredOffset] = true;
  }
}

void AllDifferentExcept::recompute(Timestamp ts) {
  for (CommittableInt& c : _counts) {
    c.setValue(ts, 0);
  }

  Int violInc = 0;
  for (size_t i = 0; i < _vars.size(); ++i) {
    const Int val = _solver.value(ts, _vars[i]);
    if (!isIgnored(val)) {
      violInc += increaseCount(ts, val);
    }
  }
  updateValue(ts, _violationId, violInc);
}

void AllDifferentExcept::notifyInputChanged(Timestamp ts, LocalId id) {
  assert(id < _committedValues.size());
  const Int newValue = _solver.value(ts, _vars[id]);
  if (newValue == _committedValues[id]) {
    return;
  }
  incValue(ts, _violationId,
               (isIgnored(_committedValues[id])
                    ? 0
                    : decreaseCount(ts, _committedValues[id])) +
               (isIgnored(newValue) ? 0 : increaseCount(ts, newValue)));
}
}  // namespace atlantis::propagation