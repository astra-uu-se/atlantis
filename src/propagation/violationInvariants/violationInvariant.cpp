#include "atlantis/propagation/violationInvariants/violationInvariant.hpp"

#include "atlantis/propagation/solver.hpp"

namespace atlantis::propagation {

ViolationInvariant::ViolationInvariant(SolverBase& solver, VarId violationId,
                                       Int nullState)
    : Invariant(solver, nullState), _violationId(violationId) {}

ViolationInvariant::ViolationInvariant(SolverBase& solver,
                                       VarViewId violationId, Int nullState)
    : ViolationInvariant(solver, VarId(violationId), nullState) {
  assert(violationId.isVar());
}

inline VarId ViolationInvariant::violationId() const { return _violationId; }

inline Int ViolationInvariant::violationCount(Timestamp& ts) const {
  return _solver.value(ts, _violationId);
}
}  // namespace atlantis::propagation
