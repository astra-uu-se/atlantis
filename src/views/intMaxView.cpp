#include "views/intMaxView.hpp"

// TODO: invariant should take its true id in the constructor.
extern Id NULL_ID;

bool IntMaxView::hasChanged(Timestamp t) {
  return m_engine.hasChanged(t, m_parentId);
}

Int IntMaxView::getValue(Timestamp t) {
  return std::max<Int>(m_max, m_engine.getValue(t, m_parentId));
}

Int IntMaxView::getCommittedValue() {
  return std::max<Int>(m_max, m_engine.getCommittedValue(m_parentId));
}

Timestamp IntMaxView::getTmpTimestamp() {
  return m_engine.getTmpTimestamp(m_parentId);
}

void IntMaxView::init(Engine& e) { m_engine = e; }