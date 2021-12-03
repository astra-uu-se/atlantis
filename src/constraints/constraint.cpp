#include "constraints/constraint.hpp"

#include "core/engine.hpp"

inline VarId Constraint::getViolationId() { return _violationId; }

inline Int Constraint::getViolationCount(Engine& engine, Timestamp& ts) {
  return engine.getValue(ts, _violationId);
}