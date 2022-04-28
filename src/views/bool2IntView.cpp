#include "views/bool2IntView.hpp"

#include "core/engine.hpp"

static inline Int convert(Int value) { return 1 - (value == 0); }

Int Bool2IntView::value(Timestamp ts) const {
  assert(0 >= _engine->lowerBound(_parentId));
  assert(_engine->upperBound(_parentId) <= 1);
  return convert(_engine->value(ts, _parentId));
}

Int Bool2IntView::committedValue() const {
  assert(0 >= _engine->lowerBound(_parentId));
  assert(_engine->upperBound(_parentId) <= 1);
  return convert(_engine->committedValue(_parentId));
}
