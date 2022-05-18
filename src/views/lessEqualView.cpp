#include "views/lessEqualView.hpp"

static inline Int compute(Int var, Int val) {
  return std::max<Int>(0, var - val);
}

Int LessEqualView::value(Timestamp ts) const {
  return compute(_engine->value(ts, _parentId), _val);
}

Int LessEqualView::committedValue() const {
  return compute(_engine->committedValue(_parentId), _val);
}

Int LessEqualView::lowerBound() const {
  return compute(_engine->lowerBound(_parentId), _val);
}

Int LessEqualView::upperBound() const {
  return compute(_engine->upperBound(_parentId), _val);
}
