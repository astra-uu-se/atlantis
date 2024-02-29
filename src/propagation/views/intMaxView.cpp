#include "atlantis/propagation/views/intMaxView.hpp"

namespace atlantis::propagation {

Int IntMaxView::value(Timestamp ts) {
  return std::max<Int>(_max, _solver.value(ts, _parentId));
}

Int IntMaxView::committedValue() {
  return std::max<Int>(_max, _solver.committedValue(_parentId));
}

Int IntMaxView::lowerBound() const {
  return std::max<Int>(_max, _solver.lowerBound(_parentId));
}

Int IntMaxView::upperBound() const {
  return std::max<Int>(_max, _solver.upperBound(_parentId));
}

}  // namespace atlantis::propagation
