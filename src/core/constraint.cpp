#include "core/engine.hpp"
#include "core/constraint.hpp"

inline VarId Constraint::getViolationId() {
  return m_violationId;
}

inline Int Constraint::getViolationCount(Engine& e, Timestamp& t) {
  return e.getValue(t, m_violationId);
}