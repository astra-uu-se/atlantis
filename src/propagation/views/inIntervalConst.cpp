#include "propagation/views/inIntervalConst.hpp"

namespace atlantis::propagation {

static inline Int isViolating(Int val, Int lb, int ub) {
  return static_cast<Int>(val < lb || ub < val);
}

Int InIntervalConst::value(Timestamp ts) {
  return isViolating(_solver.value(ts, _parentId), _lb, _ub);
}

Int InIntervalConst::committedValue() {
  return isViolating(_solver.committedValue(_parentId), _lb, _ub);
}

Int InIntervalConst::lowerBound() const {
  return isViolating(_solver.upperBound(_parentId), _lb, _ub);
}

Int InIntervalConst::upperBound() const {
  return isViolating(_solver.lowerBound(_parentId), _lb, _ub);
}

}  // namespace atlantis::propagation