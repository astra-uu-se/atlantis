#include "constraints/constraint.hpp"

#include "core/engine.hpp"

inline VarId Constraint::violationId() { return _violationId; }

inline Int Constraint::violationCount(Engine& engine, Timestamp& ts) {
  return engine.value(ts, _violationId);
}