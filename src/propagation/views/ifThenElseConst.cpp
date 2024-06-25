#include "atlantis/propagation/views/ifThenElseConst.hpp"

namespace atlantis::propagation {

IfThenElseConst::IfThenElseConst(SolverBase& solver, VarId parentId,
                                 Int thenVal, Int elseVal)
    : IntView(solver, parentId), _values{thenVal, elseVal} {}

Int IfThenElseConst::value(Timestamp ts) {
  return _values[_solver.value(ts, _parentId) == 0 ? 0 : 1];
}

Int IfThenElseConst::committedValue() {
  return _values[_solver.committedValue(_parentId) == 0 ? 0 : 1];
}

Int IfThenElseConst::lowerBound() const {
  if (_solver.upperBound(_parentId) <= 0) {
    // always true, take then case:
    return _values[0];
  } else if (_solver.lowerBound(_parentId) > 0) {
    // always false, take else case:
    return _values[1];
  }
  return std::min(_values[0], _values[1]);
}

Int IfThenElseConst::upperBound() const {
  if (_solver.upperBound(_parentId) <= 0) {
    // always true, take then case:
    return _values[0];
  } else if (_solver.lowerBound(_parentId) > 0) {
    // always false, take else case:
    return _values[1];
  }
  return std::max(_values[0], _values[1]);
}

}  // namespace atlantis::propagation
