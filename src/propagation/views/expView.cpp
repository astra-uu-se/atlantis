#include "atlantis/propagation/views/expView.hpp"

#include <initializer_list>

namespace atlantis::propagation {

Int ExpView::value(Timestamp ts) {
  return pow(_solver.value(ts, _parentId), _power);
}

Int ExpView::committedValue() {
  return pow(_solver.committedValue(_parentId), _power);
}

Int ExpView::lowerBound() const {
  return std::min(
      std::initializer_list<Int>{0, pow(_power, _solver.lowerBound(_parentId)),
                                 pow(_power, _solver.upperBound(_parentId))});
}

Int ExpView::upperBound() const {
  return std::max(pow(_power, _solver.lowerBound(_parentId)),
                  pow(_power, _solver.upperBound(_parentId)));
}

}  // namespace atlantis::propagation
