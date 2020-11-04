#include "views/intOffsetView.hpp"

// TODO: invariant should take its true id in the constructor.
extern Id NULL_ID;

Int IntOffsetView::getValue(Timestamp t) {
  return m_offset + m_engine->getValue(t, m_parentId);
}

Int IntOffsetView::getCommittedValue() {
  return m_offset + m_engine->getCommittedValue(m_parentId);
}