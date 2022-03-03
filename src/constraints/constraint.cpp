#include "constraints/constraint.hpp"

#include "core/engine.hpp"

inline VarId Constraint::getViolationId() const { return _violationId; }

inline Int Constraint::getViolationCount(Engine& engine, Timestamp& ts) const {
  return engine.getValue(ts, _violationId);
}