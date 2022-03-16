#include "views/intMaxView.hpp"

#include "core/engine.hpp"
Int IntMaxView::value(Timestamp ts) const {
  return std::max<Int>(_max, _engine->value(ts, _parentId));
}

Int IntMaxView::committedValue() const {
  return std::max<Int>(_max, _engine->committedValue(_parentId));
}

Int IntMaxView::lowerBound() const { return _engine->lowerBound(_parentId); }

Int IntMaxView::upperBound() const {
  return std::min<Int>(_max, _engine->upperBound(_parentId));
}
