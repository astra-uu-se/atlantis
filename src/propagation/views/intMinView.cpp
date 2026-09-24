#include "atlantis/propagation/views/intMinView.hpp"

#include <algorithm>

#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::propagation {

IntMinView::IntMinView(SolverBase& solver, const VarViewId parentId,
                       const Int min)
    : IntView(solver, parentId), _min(min) {}

Int IntMinView::value(const Timestamp ts) {
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
