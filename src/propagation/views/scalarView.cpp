#include "atlantis/propagation/views/scalarView.hpp"

namespace atlantis::propagation {

ScalarView::ScalarView(SolverBase& solver, VarViewId parentId, Int factor,
                       Int offset)
    : IntView(solver, parentId), _factor(factor), _offset(offset) {}

Int ScalarView::value(Timestamp ts) {
  return _factor * _solver.value(ts, _parentId) + _offset;
}

Int ScalarView::committedValue() {
  return _factor * _solver.committedValue(_parentId) + _offset;
}

Int ScalarView::lowerBound() const {
  return std::min(_factor * _solver.lowerBound(_parentId) + _offset,
                  _factor * _solver.upperBound(_parentId) + _offset);
}

Int ScalarView::upperBound() const {
  return std::max(_factor * _solver.lowerBound(_parentId) + _offset,
                  _factor * _solver.upperBound(_parentId) + _offset);
}

}  // namespace atlantis::propagation
