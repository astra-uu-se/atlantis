#include "atlantis/propagation/views/equalConst.hpp"

#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/utils/overflow.hpp"

namespace atlantis::propagation {

static Int compute(const Int var, const Int val) {
  return overflow::saturatingAbsDiff(var, val);
}

EqualConst::EqualConst(SolverBase& solver, const VarViewId parentId,
                       const Int val)
    : IntView(solver, parentId), _val(val) {}

Int EqualConst::value(const Timestamp ts) {
  return compute(_solver.value(ts, _parentId), _val);
}

Int EqualConst::committedValue() {
  return compute(_solver.committedValue(_parentId), _val);
}

Int EqualConst::lowerBound() const {
  const Int lb = _solver.lowerBound(_parentId);
  const Int ub = _solver.upperBound(_parentId);
  if (lb <= _val && _val <= ub) {
    return 0;
  }
  return std::min(compute(lb, _val), compute(ub, _val));
}

Int EqualConst::upperBound() const {
  return std::max(compute(_solver.lowerBound(_parentId), _val),
                  compute(_solver.upperBound(_parentId), _val));
}

}  // namespace atlantis::propagation
