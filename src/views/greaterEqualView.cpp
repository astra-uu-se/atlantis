#include "views/greaterEqualView.hpp"

static inline Int compute(Int var, Int val) {
  return std::max<Int>(0, val - var);
}

Int GreaterEqualView::value(Timestamp ts) {
  return compute(_engine->value(ts, _parentId), _val);
}

Int GreaterEqualView::committedValue() {
  return compute(_engine->committedValue(_parentId), _val);
}

Int GreaterEqualView::lowerBound() const {
  return compute(_engine->upperBound(_parentId), _val);
}

Int GreaterEqualView::upperBound() const {
  return compute(_engine->lowerBound(_parentId), _val);
}
