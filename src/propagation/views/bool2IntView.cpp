#include "atlantis/propagation/views/bool2IntView.hpp"

namespace atlantis::propagation {

static inline Int convert(Int value) { return static_cast<Int>(value == 0); }

Bool2IntView::Bool2IntView(SolverBase& solver, const VarViewId parentId)
    : IntView(solver, parentId) {}

Int Bool2IntView::value(Timestamp ts) {
  assert(0 <= _solver.value(ts, _parentId));
  return convert(_solver.value(ts, _parentId));
}

Int Bool2IntView::committedValue() {
  assert(0 <= _solver.committedValue(_parentId));
  return convert(_solver.committedValue(_parentId));
}

Int Bool2IntView::lowerBound() const {
  return (_solver.lowerBound(_parentId) == 0 &&
          _solver.upperBound(_parentId) == 0)
             ? 1
             : 0;
}

Int Bool2IntView::upperBound() const {
  return (_solver.lowerBound(_parentId) <= 0 &&
          _solver.upperBound(_parentId) >= 0)
             ? 1
             : 0;
}

}  // namespace atlantis::propagation
