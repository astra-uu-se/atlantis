#include "views/greaterEqualConst.hpp"

static inline Int compute(Int var, Int val) {
  return std::max<Int>(0, val - var);
}

Int GreaterEqualConst::value(Timestamp ts) {
  return compute(_engine->value(ts, _parentId), _val);
}

Int GreaterEqualConst::committedValue() {
  return compute(_engine->committedValue(_parentId), _val);
}

Int GreaterEqualConst::lowerBound() const {
  return compute(_engine->upperBound(_parentId), _val);
}

Int GreaterEqualConst::upperBound() const {
  return compute(_engine->lowerBound(_parentId), _val);
}
