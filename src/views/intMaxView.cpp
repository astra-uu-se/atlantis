#include "views/intMaxView.hpp"

#include "core/engine.hpp"
Int IntMaxView::getValue(Timestamp ts) const {
  return std::max<Int>(_max, _engine->getValue(ts, _parentId));
}

Int IntMaxView::getCommittedValue() const {
  return std::max<Int>(_max, _engine->getCommittedValue(_parentId));
}

Int IntMaxView::getLowerBound() const {
  return _engine->getLowerBound(_parentId);
}

Int IntMaxView::getUpperBound() const {
  return std::min<Int>(_max, _engine->getUpperBound(_parentId));
}
