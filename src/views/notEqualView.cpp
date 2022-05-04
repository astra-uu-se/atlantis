#include "views/notEqualView.hpp"

Int NotEqualView::value(Timestamp ts) const {
  return _engine->value(ts, _parentId) == _val;
}

Int NotEqualView::committedValue() const {
  return _engine->committedValue(_parentId) == _val;
}

Int NotEqualView::lowerBound() const {
  if (_val == _engine->lowerBound(_parentId) &&
      _val == _engine->upperBound(_parentId)) {
    return 1;
  }
  return 0;
}

Int NotEqualView::upperBound() const {
  if (_val < _engine->lowerBound(_parentId) ||
      _val > _engine->upperBound(_parentId)) {
    return 0;
  }
  return 1;
}
