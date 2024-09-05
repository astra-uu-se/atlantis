#include "atlantis/propagation/views/modView.hpp"

namespace atlantis::propagation {

ModView::ModView(SolverBase& solver, VarViewId parentId, Int denominator)
    : IntView(solver, parentId), _denominator(std::abs(denominator)) {
  if (_denominator == 0) {
    throw std::invalid_argument("Denominator cannot be zero");
  }
}

Int ModView::value(Timestamp ts) {
  return _solver.value(ts, _parentId) % _denominator;
}

Int ModView::committedValue() {
  return _solver.committedValue(_parentId) % _denominator;
}

Int ModView::lowerBound() const { return 0; }

Int ModView::upperBound() const { return _denominator - 1; }

}  // namespace atlantis::propagation
