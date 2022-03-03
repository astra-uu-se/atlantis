#include "views/intOffsetView.hpp"

#include "core/engine.hpp"

extern Id NULL_ID;

Int IntOffsetView::getValue(Timestamp ts) const {
  return _offset + _engine->getValue(ts, _parentId);
}

Int IntOffsetView::getCommittedValue() const {
  return _offset + _engine->getCommittedValue(_parentId);
}

Int IntOffsetView::getLowerBound() const {
  return _offset + _engine->getLowerBound(_parentId);
}

Int IntOffsetView::getUpperBound() const {
  return _offset + _engine->getUpperBound(_parentId);
}
