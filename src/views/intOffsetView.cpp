#include "views/intOffsetView.hpp"

#include "core/engine.hpp"

extern Id NULL_ID;

Int IntOffsetView::value(Timestamp ts) {
  return _offset + _engine->value(ts, _parentId);
}

Int IntOffsetView::committedValue() {
  return _offset + _engine->committedValue(_parentId);
}

Int IntOffsetView::lowerBound() {
  return _offset + _engine->lowerBound(_parentId);
}

Int IntOffsetView::upperBound() {
  return _offset + _engine->upperBound(_parentId);
}
