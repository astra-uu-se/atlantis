#include "propagation/constraints/allDifferentExcept.hpp"

namespace atlantis::propagation {

/**
 * @param violationId id for the violationCount
 */
AllDifferentExcept::AllDifferentExcept(Engine& engine, VarId violationId,
                                       std::vector<VarId> variables,
                                       const std::vector<Int>& ignored)
    : AllDifferent(engine, violationId, variables) {
  _modifiedVars.reserve(_variables.size());
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
  for (size_t i = 0; i < _variables.size(); ++i) {
    const Int val = _engine.value(ts, _variables[i]);
    if (!isIgnored(val)) {
      violInc += increaseCount(ts, val);
    }
  }
  updateValue(ts, _violationId, violInc);
}

void AllDifferentExcept::notifyInputChanged(Timestamp ts, LocalId id) {
  assert(id < _committedValues.size());
  const Int newValue = _engine.value(ts, _variables[id]);
  if (newValue == _committedValues[id]) {
    return;
  }
  incValue(ts, _violationId,
           static_cast<Int>(
               (isIgnored(_committedValues[id])
                    ? 0
                    : decreaseCount(ts, _committedValues[id])) +
               (isIgnored(newValue) ? 0 : increaseCount(ts, newValue))));
}
}  // namespace atlantis::propagation