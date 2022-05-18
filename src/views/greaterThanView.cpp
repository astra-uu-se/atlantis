#include "views/greaterThanView.hpp"

static inline Int compute(Int var, Int val) {
  return std::max(Int(0), val + 1 - var);
}

Int GreaterThanView::value(Timestamp ts) const {
  return compute(_engine->value(ts, _parentId), _val);
}

Int GreaterThanView::committedValue() const {
  return compute(_engine->committedValue(_parentId), _val);
}

Int GreaterThanView::lowerBound() const {
  return compute(_engine->lowerBound(_parentId), _val);
}

Int GreaterThanView::upperBound() const {
  return compute(_engine->upperBound(_parentId), _val);
}
