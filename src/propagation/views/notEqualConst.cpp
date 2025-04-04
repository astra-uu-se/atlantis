#include "atlantis/propagation/views/notEqualConst.hpp"

#include "atlantis/propagation/solverBase.hpp"

namespace atlantis::propagation {

Int NotEqualConst::value(Timestamp ts) {
  return _solver.value(ts, _parentId) == _val ? 1 : 0;
}

Int NotEqualConst::committedValue() {
  return _solver.committedValue(_parentId) == _val ? 1 : 0;
}

Int NotEqualConst::lowerBound() const {
  if (_val == _solver.lowerBound(_parentId) &&
      _val == _solver.upperBound(_parentId)) {
    return 1;
  }
  return 0;
}

Int NotEqualConst::upperBound() const {
  if (_val < _solver.lowerBound(_parentId) ||
      _val > _solver.upperBound(_parentId)) {
    return 0;
  }
  return 1;
}

}  // namespace atlantis::propagation
