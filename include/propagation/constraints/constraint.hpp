#pragma once

#include "types.hpp"
#include "propagation/invariants/invariant.hpp"

namespace atlantis::propagation {

class SolverBase;  // Forward declaration

class Constraint : public Invariant {
 private:
 protected:
  const VarId _violationId;
  explicit Constraint(SolverBase& solver, VarId violationId, Int nullState = -1)
      : Invariant(solver, nullState), _violationId(violationId) {}

 public:
  [[nodiscard]] inline VarId violationId() const;
  [[nodiscard]] inline Int violationCount(Timestamp&) const;
};

}  // namespace atlantis::propagation