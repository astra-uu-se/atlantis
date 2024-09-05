#include "atlantis/propagation/violationInvariants/allDifferent.hpp"

#include <cassert>
#include <limits>

#include "atlantis/propagation/variables/intVar.hpp"

namespace atlantis::propagation {

/**
 * @param violationId id for the violationCount
 */
AllDifferent::AllDifferent(SolverBase& solver, VarId violationId,
                           std::vector<VarViewId>&& vars)
    : ViolationInvariant(solver, violationId),
      _vars(std::move(vars)),
      _counts(),
      _offset(0) {}

AllDifferent::AllDifferent(SolverBase& solver, VarViewId violationId,
                           std::vector<VarViewId>&& vars)
    : AllDifferent(solver, VarId(violationId), std::move(vars)) {
  assert(violationId.isVar());
}

void AllDifferent::registerVars() {
  assert(_id != NULL_ID);
  for (size_t i = 0; i < _vars.size(); ++i) {
    _solver.registerInvariantInput(_id, _vars[i], i, false);
  }
  registerDefinedVar(_violationId);
}

void AllDifferent::updateBounds(bool widenOnly) {
  _solver.updateBounds(_violationId, 0, static_cast<Int>(_vars.size() - 1),
                       widenOnly);
}

void AllDifferent::close(Timestamp ts) {
  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::min();

  Int overlapLb = lb;
  Int overlapUb = ub;

  for (const auto& var : _vars) {
    if (_solver.lowerBound(var) < lb) {
      assert(lb <= overlapLb);
      overlapLb = lb;
      lb = _solver.lowerBound(var);
    } else if (_solver.lowerBound(var) < overlapLb) {
      overlapLb = _solver.lowerBound(var);
    }
    if (_solver.upperBound(var) >= ub) {
      assert(ub >= overlapUb);
      overlapUb = ub;
      ub = _solver.upperBound(var);
    } else if (_solver.upperBound(var) > overlapUb) {
      overlapUb = _solver.upperBound(var);
    }
  }
  if (overlapUb < overlapLb) {
    _counts.clear();
  } else {
    _counts.resize(static_cast<unsigned long>(overlapUb - overlapLb + 1),
                   CommittableInt(ts, 0));
  }
  _offset = overlapLb;
}

void AllDifferent::recompute(Timestamp ts) {
  for (CommittableInt& c : _counts) {
    c.setValue(ts, 0);
  }

  Int violInc = 0;
  for (const auto& var : _vars) {
    violInc += increaseCount(ts, _solver.value(ts, var));
  }
  updateValue(ts, _violationId, violInc);
}

void AllDifferent::notifyInputChanged(Timestamp ts, LocalId id) {
  assert(id < _vars.size());
  const Int newValue = _solver.value(ts, _vars[id]);
  const Int committedValue = _solver.committedValue(_vars[id]);
  if (newValue == committedValue) {
    return;
  }
  incValue(ts, _violationId,
           static_cast<Int>(decreaseCount(ts, committedValue) +
                            increaseCount(ts, newValue)));
}

VarViewId AllDifferent::nextInput(Timestamp ts) {
  const auto index = static_cast<size_t>(_state.incValue(ts, 1));
  if (index < _vars.size()) {
    return _vars[index];
  }
  return NULL_ID;
}

void AllDifferent::notifyCurrentInputChanged(Timestamp ts) {
  assert(static_cast<size_t>(_state.value(ts)) < _vars.size());
  notifyInputChanged(ts, static_cast<size_t>(_state.value(ts)));
}

void AllDifferent::commit(Timestamp ts) {
  Invariant::commit(ts);

  for (CommittableInt& committableInt : _counts) {
    committableInt.commitIf(ts);
  }
}

}  // namespace atlantis::propagation
