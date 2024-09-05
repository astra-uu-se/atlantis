#include "atlantis/propagation/views/inIntervalConst.hpp"

namespace atlantis::propagation {

static inline Int compute(Int val, Int lb, Int ub) {
  return lb <= val && val <= ub ? 0 : 1;
}

InIntervalConst::InIntervalConst(SolverBase& solver, VarViewId parentId, Int lb,
                                 Int ub)
    : IntView(solver, parentId), _lb(lb), _ub(ub) {}

Int InIntervalConst::value(Timestamp ts) {
  const Int val = compute(_solver.value(ts, _parentId), _lb, _ub);
  return val;
}

Int InIntervalConst::committedValue() {
  return compute(_solver.committedValue(_parentId), _lb, _ub);
}

Int InIntervalConst::lowerBound() const {
  const Int lb = _solver.lowerBound(_parentId);
  const Int ub = _solver.upperBound(_parentId);
  if (ub < _lb || _ub < lb) {
    return 1;
  }
  return 0;
}

Int InIntervalConst::upperBound() const {
  const Int lb = _solver.lowerBound(_parentId);
  const Int ub = _solver.upperBound(_parentId);
  if (_lb <= lb && ub <= _ub) {
    return 0;
  }
  return 1;
}

}  // namespace atlantis::propagation
