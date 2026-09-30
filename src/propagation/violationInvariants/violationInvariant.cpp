#include "atlantis/propagation/violationInvariants/violationInvariant.hpp"

#include "atlantis/propagation/solver.hpp"

namespace atlantis::propagation {

ViolationInvariant::ViolationInvariant(SolverBase& solver,
                                       const VarId violationId,
                                       const Int nullState)
    : Invariant(solver, nullState), _violationId(violationId) {}

ViolationInvariant::ViolationInvariant(SolverBase& solver,
                                       const VarViewId violationId,
                                       const Int nullState)
    : ViolationInvariant(solver, VarId{violationId}, nullState) {
  assert(violationId.isVar());
}

inline VarId ViolationInvariant::violationId() const { return _violationId; }

inline Int ViolationInvariant::violationCount(const Timestamp ts) const {
  return _solver.value(ts, _violationId);
}
}  // namespace atlantis::propagation
