#include "atlantis/propagation/views/intOffsetView.hpp"

namespace atlantis::propagation {

Int IntOffsetView::value(Timestamp ts) {
  return _offset + _solver.value(ts, _parentId);
}

Int IntOffsetView::committedValue() {
  return _offset + _solver.committedValue(_parentId);
}

Int IntOffsetView::lowerBound() const {
  return _offset + _solver.lowerBound(_parentId);
}

Int IntOffsetView::upperBound() const {
  return _offset + _solver.upperBound(_parentId);
}

}  // namespace atlantis::propagation
