#include "atlantis/propagation/views/elementConst.hpp"

#include <algorithm>
#include <limits>

namespace atlantis::propagation {

ElementConst::ElementConst(SolverBase& solver, VarId parentId,
                           std::vector<Int>&& array, Int offset)
    : IntView(solver, parentId), _array(std::move(array)), _offset(offset) {}

Int ElementConst::value(Timestamp ts) {
  assert(safeIndex(_solver.value(ts, _parentId)) < _array.size());
  return _array[safeIndex(_solver.value(ts, _parentId))];
}

Int ElementConst::committedValue() {
  assert(safeIndex(_solver.committedValue(_parentId)) < _array.size());
  return _array[safeIndex(_solver.committedValue(_parentId))];
}

Int ElementConst::lowerBound() const {
  const Int indexBegin =
      std::max<Int>(0, _solver.lowerBound(_parentId) - _offset);
  const Int indexEnd =
      std::min<Int>(static_cast<Int>(_array.size()),
                    _solver.upperBound(_parentId) - _offset + 1);
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
      std::max<Int>(0, _solver.lowerBound(_parentId) - _offset);
  const Int indexEnd =
      std::min<Int>(static_cast<Int>(_array.size()),
                    _solver.upperBound(_parentId) - _offset + 1);

  if (indexBegin >= static_cast<Int>(_array.size())) {
    return _array.back();
  } else if (indexEnd < 0) {
    return _array.front();
  }
  return *std::max_element(_array.begin() + indexBegin,
                           _array.begin() + indexEnd);
}
}  // namespace atlantis::propagation
