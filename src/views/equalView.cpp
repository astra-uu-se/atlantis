#include "views/equalView.hpp"

Int EqualView::value(Timestamp ts) {
  return std::abs(_engine->value(ts, _parentId) - _val);
}

Int EqualView::committedValue() {
  return std::abs(_engine->committedValue(_parentId) - _val);
}

Int EqualView::lowerBound() const {
  const Int lb = _engine->lowerBound(_parentId);
  const Int ub = _engine->upperBound(_parentId);
  if (lb <= _val && _val <= ub) {
    return Int(0);
  }
  return std::min(std::abs(lb - _val), std::abs(ub - _val));
}

Int EqualView::upperBound() const {
  return std::max(std::abs(_engine->lowerBound(_parentId) - _val),
                  std::abs(_engine->upperBound(_parentId) - _val));
}
