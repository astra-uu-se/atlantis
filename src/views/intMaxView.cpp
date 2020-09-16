#include "views/intMaxView.hpp"

// TODO: invariant should take its true id in the constructor.
extern Id NULL_ID;

IntMaxView::IntMaxView(const VarId& t_parentId, Int t_max)
    : IntVarView(t_parentId), m_max(t_max) {}

inline void IntMaxView::init(Timestamp t, Engine&, const Int& parentVal, const Int& parentCommittedVal) {
  m_savedInt.setValue(t, std::min(
    m_max,
    parentVal
  ));
  m_savedInt.commitValue(std::min(
    m_max,
    parentCommittedVal
  ));
}

inline void IntMaxView::recompute(Timestamp t, Engine& e) {
  if (m_savedInt.getTmpTimestamp() != t) {
    m_savedInt.commitValue(std::min(m_max, e.getCommittedValue(m_parentId)));
    m_savedInt.setValue(t, std::min(m_max, e.getValue(t, m_parentId)));
  }
}

inline void IntMaxView::recompute(Timestamp t, const Int& parentVal) {
  m_savedInt.setValue(t, std::min(m_max, parentVal));
}

inline void IntMaxView::recompute(Timestamp t, const Int& parentVal, const Int& parentCommittedVal) {
  m_savedInt.setValue(t, std::min(
    m_max,
    parentVal
  ));
  m_savedInt.commitValue(std::min(
    m_max,
    parentCommittedVal
  ));
}

inline void IntMaxView::commitValue(const Int& parentVal) {
  m_savedInt.commitValue(std::min(m_max, parentVal));
}