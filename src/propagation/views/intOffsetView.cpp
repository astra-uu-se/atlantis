#include "propagation/views/intOffsetView.hpp"

namespace atlantis::propagation {

Int IntOffsetView::value(Timestamp ts) {
  return _offset + _engine.value(ts, _parentId);
}

Int IntOffsetView::committedValue() {
  return _offset + _engine.committedValue(_parentId);
}

Int IntOffsetView::lowerBound() const {
  return _offset + _engine.lowerBound(_parentId);
}

Int IntOffsetView::upperBound() const {
  return _offset + _engine.upperBound(_parentId);
}

}  // namespace atlantis::propagation