#include "propagation/views/notEqualConst.hpp"

namespace atlantis::propagation {

Int NotEqualConst::value(Timestamp ts) {
  return static_cast<Int>(_solver.value(ts, _parentId) == _val);
}

Int NotEqualConst::committedValue() {
  return static_cast<Int>(_solver.committedValue(_parentId) == _val);
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