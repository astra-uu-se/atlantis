#include "propagation/views/intAbsView.hpp"

namespace atlantis::propagation {

Int IntAbsView::value(Timestamp ts) {
  return std::abs(_engine.value(ts, _parentId));
}

Int IntAbsView::committedValue() {
  return std::abs(_engine.committedValue(_parentId));
}

Int IntAbsView::lowerBound() const {
  const Int ub = _engine.upperBound(_parentId);
  // the values of the source are always negative:
  if (ub < 0) {
    return -ub;
  }
  const Int lb = _engine.lowerBound(_parentId);
  // lb <= 0 <= ub:
  if (lb <= 0) {
    return 0;
  }
  // The values of the source are always positive
  return lb;
}

Int IntAbsView::upperBound() const {
  return std::max(std::abs(_engine.lowerBound(_parentId)),
                  _engine.upperBound(_parentId));
}

}  // namespace atlantis::propagation