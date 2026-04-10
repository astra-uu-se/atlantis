#include "atlantis/propagation/views/intOffsetView.hpp"

#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/utils/overflow.hpp"

namespace atlantis::propagation {

IntOffsetView::IntOffsetView(SolverBase& solver, VarViewId parentId, Int offset)
    : IntView(solver, parentId), _offset(offset) {}

Int IntOffsetView::value(Timestamp ts) {
  return overflow::saturatingAdd(_offset, _solver.value(ts, _parentId));
}

Int IntOffsetView::committedValue() {
  return overflow::saturatingAdd(_offset, _solver.committedValue(_parentId));
}

Int IntOffsetView::lowerBound() const {
  return overflow::saturatingAdd(_offset, _solver.lowerBound(_parentId));
}

Int IntOffsetView::upperBound() const {
  return overflow::saturatingAdd(_offset, _solver.upperBound(_parentId));
}

}  // namespace atlantis::propagation
