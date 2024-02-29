#include "atlantis/propagation/views/scalarView.hpp"

namespace atlantis::propagation {

Int ScalarView::value(Timestamp ts) {
  return _scalar * _solver.value(ts, _parentId);
}

Int ScalarView::committedValue() {
  return _scalar * _solver.committedValue(_parentId);
}

Int ScalarView::lowerBound() const {
  return std::min(_scalar * _solver.lowerBound(_parentId),
                  _scalar * _solver.upperBound(_parentId));
}

Int ScalarView::upperBound() const {
  return std::max(_scalar * _solver.lowerBound(_parentId),
                  _scalar * _solver.upperBound(_parentId));
}

}  // namespace atlantis::propagation
