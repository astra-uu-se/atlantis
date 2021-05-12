#include "constraints/constraint.hpp"

#include "core/engine.hpp"

inline VarId Constraint::getViolationId() { return _violationId; }

inline Int Constraint::getViolationCount(Engine& e, Timestamp& t) {
  return e.getValue(t, _violationId);
}