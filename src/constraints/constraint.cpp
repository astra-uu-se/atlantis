#include "constraints/constraint.hpp"

#include "core/engine.hpp"

inline VarId Constraint::violationId() const { return _violationId; }

inline Int Constraint::violationCount(Timestamp& ts) const {
  return _engine.value(ts, _violationId);
}