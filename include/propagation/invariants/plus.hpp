#pragma once
#include <cmath>

#include "types.hpp"
#include "propagation/solver.hpp"
#include "propagation/invariants/invariant.hpp"

namespace atlantis::propagation {

/**
 * Invariant for output <- x + y
 *
 */
class Plus : public Invariant {
 private:
  VarId _output, _x, _y;

 public:
  explicit Plus(SolverBase&, VarId output, VarId x, VarId y);
  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation