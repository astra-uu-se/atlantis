#include "atlantis/propagation/views/intAbsView.hpp"

namespace atlantis::propagation {

Int IntAbsView::value(Timestamp ts) {
  return std::abs(_solver.value(ts, _parentId));
}

Int IntAbsView::committedValue() {
  return std::abs(_solver.committedValue(_parentId));
}

Int IntAbsView::lowerBound() const {
  const Int ub = _solver.upperBound(_parentId);
  // the values of the source are always negative:
  if (ub < 0) {
    return -ub;
  }
  const Int lb = _solver.lowerBound(_parentId);
  // lb <= 0 <= ub:
  if (lb <= 0) {
    return 0;
  }
  // The values of the source are always positive
  return lb;
}

Int IntAbsView::upperBound() const {
  return std::max(std::abs(_solver.lowerBound(_parentId)),
                  _solver.upperBound(_parentId));
}

}  // namespace atlantis::propagation
