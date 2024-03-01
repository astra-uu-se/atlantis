#pragma once

#include "atlantis/propagation/invariants/invariant.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/types.hpp"

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
