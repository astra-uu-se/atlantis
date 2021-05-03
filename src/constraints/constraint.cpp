#include "constraints/constraint.hpp"

#include "core/engine.hpp"

inline VarId Constraint::getViolationId() { return m_violationId; }

inline Int Constraint::getViolationCount(Engine& e, Timestamp& t) {
  return e.getValue(t, m_violationId);
}