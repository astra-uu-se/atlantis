#pragma once

#include "propagation/invariants/invariant.hpp"
#include "propagation/solver.hpp"
#include "propagation/variables/intVar.hpp"
#include "types.hpp"

namespace atlantis::propagation {

/**
 * Invariant for output <- x /\ y
 *
 */
class BoolAnd : public Invariant {
 private:
  VarId _output, _x, _y;

 public:
  explicit BoolAnd(SolverBase&, VarId output, VarId x, VarId y);
  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation