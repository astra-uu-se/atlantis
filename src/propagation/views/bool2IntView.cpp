#include "propagation/views/bool2IntView.hpp"

namespace atlantis::propagation {

static inline Int convert(Int value) { return static_cast<Int>(value == 0); }

Int Bool2IntView::value(Timestamp ts) {
  assert(0 >= _engine.lowerBound(_parentId));
  return convert(_engine.value(ts, _parentId));
}

Int Bool2IntView::committedValue() {
  assert(0 >= _engine.lowerBound(_parentId));
  return convert(_engine.committedValue(_parentId));
}
}  // namespace atlantis::propagation