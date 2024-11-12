#include "atlantis/propagation/views/int2BoolView.hpp"

namespace atlantis::propagation {

static inline Int convert(Int value) { return value <= 0 ? 1 : 0; }

Int2BoolView::Int2BoolView(SolverBase& solver, const VarViewId parentId)
    : IntView(solver, parentId) {}

Int Int2BoolView::value(Timestamp ts) {
  return convert(_solver.value(ts, _parentId));
}

Int Int2BoolView::committedValue() {
  return convert(_solver.committedValue(_parentId));
}

Int Int2BoolView::lowerBound() const {
  // The integer can be positive <-> the violation can be 0:
  return _solver.upperBound(_parentId) > 0 ? 0 : 1;
}
Int Int2BoolView::upperBound() const {
  // The integer can be negative <-> the violation can be 1:
  return _solver.lowerBound(_parentId) <= 0 ? 1 : 0;
}

}  // namespace atlantis::propagation
