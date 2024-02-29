#include "atlantis/propagation/views/greaterEqualConst.hpp"

namespace atlantis::propagation {

static inline Int compute(Int var, Int val) {
  return std::max<Int>(0, val - var);
}

Int GreaterEqualConst::value(Timestamp ts) {
  return compute(_solver.value(ts, _parentId), _val);
}

Int GreaterEqualConst::committedValue() {
  return compute(_solver.committedValue(_parentId), _val);
}

Int GreaterEqualConst::lowerBound() const {
  return compute(_solver.upperBound(_parentId), _val);
}

Int GreaterEqualConst::upperBound() const {
  return compute(_solver.lowerBound(_parentId), _val);
}

}  // namespace atlantis::propagation
