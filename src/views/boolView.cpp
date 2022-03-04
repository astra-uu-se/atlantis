#include "views/boolView.hpp"

#include "core/engine.hpp"

Int BoolView::getValue(Timestamp ts) {
  return 1 - (_engine->getValue(ts, _parentId) == 0);
}

Int BoolView::getCommittedValue() {
  return 1 - (_engine->getCommittedValue(_parentId) == 0);
}
