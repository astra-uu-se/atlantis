#pragma once

#include "propagation/invariants/invariant.hpp"
#include "types.hpp"

namespace atlantis::propagation {

class SolverBase;  // Forward declaration

class ViolationInvariant : public Invariant {
 protected:
  VarId _violationId;
  explicit ViolationInvariant(SolverBase& solver, VarId violationId,
                              Int nullState = -1)
      : Invariant(solver, nullState), _violationId(violationId) {}

 public:
  [[nodiscard]] inline VarId violationId() const;
  [[nodiscard]] inline Int violationCount(Timestamp&) const;
};

}  // namespace atlantis::propagation