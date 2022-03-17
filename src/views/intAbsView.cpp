#include "views/intAbsView.hpp"

#include "core/engine.hpp"

Int IntAbsView::value(Timestamp ts) const {
  return std::abs(_engine->value(ts, _parentId));
}

Int IntAbsView::committedValue() const {
  return std::abs(_engine->committedValue(_parentId));
}

Int IntAbsView::lowerBound() const {
  return std::min(std::abs(_engine->lowerBound(_parentId)), std::abs(_engine->upperBound(_parentId)));
}

Int IntAbsView::upperBound() const {
  return std::max(std::abs(_engine->lowerBound(_parentId)), std::abs(_engine->upperBound(_parentId)));
}
