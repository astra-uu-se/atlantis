#include "propagation/constraints/constraint.hpp"

#include "propagation/engine.hpp"

namespace atlantis::propagation {

inline VarId Constraint::violationId() const { return _violationId; }

inline Int Constraint::violationCount(Timestamp& ts) const {
  return _engine.value(ts, _violationId);
}
}  // namespace atlantis::propagation