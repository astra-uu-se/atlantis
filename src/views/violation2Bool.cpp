#include "core/engine.hpp"
#include "views/violation2BoolView.hpp"

static Int convert(Int value) { return std::min<Int>(value, 1); }

Int Violation2BoolView::value(Timestamp ts) const {
  return convert(_engine->value(ts, _parentId));
}

Int Violation2BoolView::committedValue() const {
  return convert(_engine->committedValue(_parentId));
}
