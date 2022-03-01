#include "views/intAbsView.hpp"

#include "core/engine.hpp"

Int IntAbsView::getValue(Timestamp ts) {
  return std::abs(_engine->getValue(ts, _parentId));
}

Int IntAbsView::getCommittedValue() {
  return std::abs(_engine->getCommittedValue(_parentId));
}

Int IntAbsView::getLowerBound() {
  return std::min(std::abs(_engine->getLowerBound(_parentId)), std::abs(_engine->getUpperBound(_parentId)));
}

Int IntAbsView::getUpperBound() {
  return std::max(std::abs(_engine->getLowerBound(_parentId)), std::abs(_engine->getUpperBound(_parentId)));
}
