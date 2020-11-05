#include "views/intMaxView.hpp"

Int IntMaxView::getValue(Timestamp t) {
  return std::max<Int>(m_max, m_engine->getValue(t, m_parentId));
}

Int IntMaxView::getCommittedValue() {
  return std::max<Int>(m_max, m_engine->getCommittedValue(m_parentId));
}

Int IntMaxView::getLowerBound() { return m_engine->getLowerBound(m_parentId); }

Int IntMaxView::getUpperBound() {
  return std::min<Int>(m_max, m_engine->getUpperBound(m_parentId));
}
