#include "views/elementConst.hpp"

#include "core/engine.hpp"

ElementConst::ElementConst(VarId parentId, std::vector<Int> array, Int offset)
    : IntView(parentId), _array(array), _offset(offset) {}

Int ElementConst::value(Timestamp ts) {
  assert(safeIndex(_engine->value(ts, _parentId)) < _array.size());
  return _array[safeIndex(_engine->value(ts, _parentId))];
}

Int ElementConst::committedValue() {
  assert(safeIndex(_engine->committedValue(_parentId)) < _array.size());
  return _array[safeIndex(_engine->committedValue(_parentId))];
}

Int ElementConst::lowerBound() const {
  const Int indexBegin =
      std::max<Int>(0, _engine->lowerBound(_parentId) - _offset);
  const Int indexEnd = std::min<Int>(
      _array.size(), _engine->upperBound(_parentId) - _offset + 1);
  if (indexBegin >= static_cast<Int>(_array.size())) {
    return _array.back();
  } else if (indexEnd < 0) {
    return _array.front();
  }
  return *std::min_element(_array.begin() + indexBegin,
                           _array.begin() + indexEnd);
}

Int ElementConst::upperBound() const {
  const Int indexBegin =
      std::max<Int>(0, _engine->lowerBound(_parentId) - _offset);
  const Int indexEnd = std::min<Int>(
      _array.size(), _engine->upperBound(_parentId) - _offset + 1);

  if (indexBegin >= static_cast<Int>(_array.size())) {
    return _array.back();
  } else if (indexEnd < 0) {
    return _array.front();
  }
  return *std::max_element(_array.begin() + indexBegin,
                           _array.begin() + indexEnd);
}