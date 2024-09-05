#include "atlantis/propagation/views/violation2BoolView.hpp"

namespace atlantis::propagation {

static Int convert(Int value) { return std::min<Int>(value, 1); }

Violation2BoolView::Violation2BoolView(SolverBase& solver, VarViewId parentId)
    : IntView(solver, parentId) {}

Int Violation2BoolView::value(Timestamp ts) {
  return convert(_solver.value(ts, _parentId));
}

Int Violation2BoolView::committedValue() {
  return convert(_solver.committedValue(_parentId));
}

Int Violation2BoolView::lowerBound() const { return 0; }
Int Violation2BoolView::upperBound() const { return 1; }

}  // namespace atlantis::propagation
