#include "constraints/constraint.hpp"

#include "core/engine.hpp"

inline VarId Constraint::violationId() const { return _violationId; }

inline Int Constraint::violationCount(Engine& engine, Timestamp& ts) const {
  return engine.value(ts, _violationId);
}