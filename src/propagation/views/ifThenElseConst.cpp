#include "atlantis/propagation/views/ifThenElseConst.hpp"

namespace atlantis::propagation {

IfThenElseConst::IfThenElseConst(SolverBase& solver, VarId parentId,
                                 Int thenVal, Int elseVal, Int condVal)
    : IntView(solver, parentId), _values{thenVal, elseVal}, _condVal(condVal) {}

Int IfThenElseConst::value(Timestamp ts) {
  return _values[_solver.value(ts, _parentId) == _condVal ? 0 : 1];
}

Int IfThenElseConst::committedValue() {
  return _values[_solver.committedValue(_parentId) == _condVal ? 0 : 1];
}

Int IfThenElseConst::lowerBound() const {
  if (_condVal == _solver.lowerBound(_parentId) &&
      _condVal == _solver.upperBound(_parentId)) {
    // always true, take then case:
    return _values[0];
  } else if (_condVal < _solver.lowerBound(_parentId) ||
             _solver.upperBound(_parentId) < _condVal) {
    // always false, take else case:
    return _values[1];
  }
  return std::min(_values[0], _values[1]);
}

Int IfThenElseConst::upperBound() const {
  if (_condVal == _solver.lowerBound(_parentId) &&
      _condVal == _solver.upperBound(_parentId)) {
    // always true, take then case:
    return _values[0];
  } else if (_condVal < _solver.lowerBound(_parentId) ||
             _solver.upperBound(_parentId) < _condVal) {
    // always false, take else case:
    return _values[1];
  }
  return std::max(_values[0], _values[1]);
}

}  // namespace atlantis::propagation
