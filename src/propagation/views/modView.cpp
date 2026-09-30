#include "atlantis/propagation/views/modView.hpp"

#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::propagation {

ModView::ModView(SolverBase& solver, const VarViewId parentId,
                 const Int denominator)
    : IntView(solver, parentId), _denominator(std::abs(denominator)) {
  if (_denominator == 0) {
    throw std::invalid_argument("Denominator cannot be zero");
  }
}

Int ModView::value(const Timestamp ts) {
  const Int parentVal = _solver.value(ts, _parentId);
  const Int remainder = parentVal % _denominator;
  return remainder;
}

Int ModView::committedValue() {
  return _solver.committedValue(_parentId) % _denominator;
}

Int ModView::lowerBound() const {
  return _solver.lowerBound(_parentId) >= 0
             ? 0
             : std::min(-_denominator + 1, Int{0});
}

Int ModView::upperBound() const {
  return _solver.upperBound(_parentId) <= 0
             ? 0
             : std::max(_denominator - 1, Int{0});
}

}  // namespace atlantis::propagation
