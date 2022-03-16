#include "views/boolView.hpp"

#include "core/engine.hpp"

Int BoolView::value(Timestamp ts) const {
  return 1 - (_engine->value(ts, _parentId) == 0);
}

Int BoolView::committedValue() const {
  return 1 - (_engine->committedValue(_parentId) == 0);
}
