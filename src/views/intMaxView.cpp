#include "views/intMaxView.hpp"

// TODO: invariant should take its true id in the constructor.
extern Id NULL_ID;

IntMaxView::IntMaxView(const VarId& t_sourceId, Int t_max)
    : IntVarView(t_sourceId), m_max(t_max) {}

inline void IntMaxView::init(Timestamp t, Engine&, const Int& sourceVal, const Int& sourceCommittedVal) {
  m_savedInt.setValue(t, std::min(
    m_max,
    sourceVal
  ));
  m_savedInt.commitValue(std::min(
    m_max,
    sourceCommittedVal
  ));
}

inline void IntMaxView::recompute(Timestamp t, Engine& e) {
  if (m_savedInt.getTmpTimestamp() != t) {
    m_savedInt.commitValue(std::min(m_max, e.getCommittedValue(m_sourceId)));
    m_savedInt.setValue(t, std::min(m_max, e.getValue(t, m_sourceId)));
  }
}

inline void IntMaxView::recompute(Timestamp t, const Int& sourceVal) {
  m_savedInt.setValue(t, std::min(m_max, sourceVal));
}

inline void IntMaxView::recompute(Timestamp t, const Int& sourceVal, const Int& sourceCommittedVal) {
  m_savedInt.setValue(t, std::min(
    m_max,
    sourceVal
  ));
  m_savedInt.commitValue(std::min(
    m_max,
    sourceCommittedVal
  ));
}

inline void IntMaxView::commitValue(const Int& sourceVal) {
  m_savedInt.commitValue(std::min(m_max, sourceVal));
}