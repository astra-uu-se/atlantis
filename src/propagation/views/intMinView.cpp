#include "atlantis/propagation/views/intMinView.hpp"

namespace atlantis::propagation {

IntMinView::IntMinView(SolverBase& solver, VarViewId parentId, Int min)
    : IntView(solver, parentId), _min(min) {}

Int IntMinView::value(Timestamp ts) {
  return std::min<Int>(_min, _solver.value(ts, _parentId));
}

Int IntMinView::committedValue() {
  return std::min<Int>(_min, _solver.committedValue(_parentId));
}

Int IntMinView::lowerBound() const {
  return std::min<Int>(_min, _solver.lowerBound(_parentId));
}

Int IntMinView::upperBound() const {
  return std::min<Int>(_min, _solver.upperBound(_parentId));
}

}  // namespace atlantis::propagation
