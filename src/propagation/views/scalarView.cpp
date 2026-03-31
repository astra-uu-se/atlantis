#include "atlantis/propagation/views/scalarView.hpp"

#include <algorithm>

#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/utils/overflow.hpp"

namespace atlantis::propagation {

ScalarView::ScalarView(SolverBase& solver, VarViewId parentId, Int factor,
                       Int offset)
    : IntView(solver, parentId), _factor(factor), _offset(offset) {}

Int ScalarView::value(Timestamp ts) {
  return overflow::saturatingAdd(
      overflow::saturatingMul(_factor, _solver.value(ts, _parentId)), _offset);
}

Int ScalarView::committedValue() {
  return overflow::saturatingAdd(
      overflow::saturatingMul(_factor, _solver.committedValue(_parentId)),
      _offset);
}

Int ScalarView::lowerBound() const {
  const Int fromLb = overflow::saturatingAdd(
      overflow::saturatingMul(_factor, _solver.lowerBound(_parentId)), _offset);
  const Int fromUb = overflow::saturatingAdd(
      overflow::saturatingMul(_factor, _solver.upperBound(_parentId)), _offset);
  return std::min(fromLb, fromUb);
}

Int ScalarView::upperBound() const {
  const Int fromLb = overflow::saturatingAdd(
      overflow::saturatingMul(_factor, _solver.lowerBound(_parentId)), _offset);
  const Int fromUb = overflow::saturatingAdd(
      overflow::saturatingMul(_factor, _solver.upperBound(_parentId)), _offset);
  return std::max(fromLb, fromUb);
}

}  // namespace atlantis::propagation
