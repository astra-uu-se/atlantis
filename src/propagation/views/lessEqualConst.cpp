#include "propagation/views/lessEqualConst.hpp"

namespace atlantis::propagation {

static inline Int compute(Int var, Int val) {
  return std::max<Int>(0, var - val);
}

Int LessEqualConst::value(Timestamp ts) {
  return compute(_engine.value(ts, _parentId), _val);
}

Int LessEqualConst::committedValue() {
  return compute(_engine.committedValue(_parentId), _val);
}

Int LessEqualConst::lowerBound() const {
  return compute(_engine.lowerBound(_parentId), _val);
}

Int LessEqualConst::upperBound() const {
  return compute(_engine.upperBound(_parentId), _val);
}

}  // namespace atlantis::propagation