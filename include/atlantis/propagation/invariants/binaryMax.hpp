#pragma once

#include "atlantis/propagation/invariants/invariant.hpp"

namespace atlantis::propagation {

/**
 * Invariant for output <- max(x, y)
 *
 */
class BinaryMax : public Invariant {
  VarId _output;
  VarViewId _x, _y;

 public:
  explicit BinaryMax(SolverBase& solver, VarId output, VarViewId x,
                     VarViewId y);
  explicit BinaryMax(SolverBase& solver, VarViewId output, VarViewId x,
                     VarViewId y);
  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarViewId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation
