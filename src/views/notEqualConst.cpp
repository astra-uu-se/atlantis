#include "views/notEqualConst.hpp"

Int NotEqualConst::value(Timestamp ts) {
  return static_cast<Int>(_engine.value(ts, _parentId) == _val);
}

Int NotEqualConst::committedValue() {
  return static_cast<Int>(_engine.committedValue(_parentId) == _val);
}

Int NotEqualConst::lowerBound() const {
  if (_val == _engine.lowerBound(_parentId) &&
      _val == _engine.upperBound(_parentId)) {
    return 1;
  }
  return 0;
}

Int NotEqualConst::upperBound() const {
  if (_val < _engine.lowerBound(_parentId) ||
      _val > _engine.upperBound(_parentId)) {
    return 0;
  }
  return 1;
}
