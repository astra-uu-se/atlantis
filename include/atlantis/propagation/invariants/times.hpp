#pragma once

#include "atlantis/propagation/invariants/invariant.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"

namespace atlantis::propagation {

/**
 * Invariant for output <- x * y
 *
 */

class Times : public Invariant {
 private:
  VarId _output;
  VarViewId _x, _y;

 public:
  explicit Times(SolverBase&, VarId output, VarViewId x, VarViewId y);

  explicit Times(SolverBase&, VarViewId output, VarViewId x, VarViewId y);

  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void recompute(Timestamp) override;
  VarViewId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
};

}  // namespace atlantis::propagation
