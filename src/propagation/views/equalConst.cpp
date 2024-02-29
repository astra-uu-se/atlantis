#include "atlantis/propagation/views/equalConst.hpp"

namespace atlantis::propagation {

inline static Int compute(Int var, Int val) { return std::abs(var - val); }

Int EqualConst::value(Timestamp ts) {
  return compute(_solver.value(ts, _parentId), _val);
}

Int EqualConst::committedValue() {
  return compute(_solver.committedValue(_parentId), _val);
}

Int EqualConst::lowerBound() const {
  const Int lb = _solver.lowerBound(_parentId);
  const Int ub = _solver.upperBound(_parentId);
  if (lb <= _val && _val <= ub) {
    return Int(0);
  }
  return std::min(compute(lb, _val), compute(ub, _val));
}

Int EqualConst::upperBound() const {
  return std::max(compute(_solver.lowerBound(_parentId), _val),
                  compute(_solver.upperBound(_parentId), _val));
}

}  // namespace atlantis::propagation
