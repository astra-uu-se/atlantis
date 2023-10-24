#include "propagation/violationInvariants/violationInvariant.hpp"

#include "propagation/solver.hpp"

namespace atlantis::propagation {

inline VarId ViolationInvariant::violationId() const { return _violationId; }

inline Int ViolationInvariant::violationCount(Timestamp& ts) const {
  return _solver.value(ts, _violationId);
}
}  // namespace atlantis::propagation