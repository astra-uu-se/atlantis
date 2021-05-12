#include "views/intMaxView.hpp"

#include "core/engine.hpp"
Int IntMaxView::getValue(Timestamp t) {
  return std::max<Int>(_max, _engine->getValue(t, _parentId));
}

Int IntMaxView::getCommittedValue() {
  return std::max<Int>(_max, _engine->getCommittedValue(_parentId));
}

Int IntMaxView::getLowerBound() { return _engine->getLowerBound(_parentId); }

Int IntMaxView::getUpperBound() {
  return std::min<Int>(_max, _engine->getUpperBound(_parentId));
}
