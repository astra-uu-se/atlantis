#include "views/scalarView.hpp"

Int ScalarView::value(Timestamp ts) {
  return _scalar * _engine.value(ts, _parentId);
}

Int ScalarView::committedValue() {
  return _scalar * _engine.committedValue(_parentId);
}

Int ScalarView::lowerBound() const {
  return std::min(_scalar * _engine.lowerBound(_parentId),
                  _scalar * _engine.upperBound(_parentId));
}

Int ScalarView::upperBound() const {
  return std::max(_scalar * _engine.lowerBound(_parentId),
                  _scalar * _engine.upperBound(_parentId));
}
