#include "propagation/views/violation2BoolView.hpp"

namespace atlantis::propagation {

static Int convert(Int value) { return std::min<Int>(value, 1); }

Int Violation2BoolView::value(Timestamp ts) {
  return convert(_engine.value(ts, _parentId));
}

Int Violation2BoolView::committedValue() {
  return convert(_engine.committedValue(_parentId));
}

}  // namespace atlantis::propagation