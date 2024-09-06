#pragma once

#include "atlantis/propagation/invariants/invariant.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"

namespace atlantis::propagation {

/**
 * Invariant for output <- xor(x, y) [bool(x) != bool(y)]
 *
 */

class BoolXor : public Invariant {
 private:
  VarId _output;
  VarViewId _x, _y;

 public:
  explicit BoolXor(SolverBase&, VarId output, VarViewId x, VarViewId y);

  explicit BoolXor(SolverBase&, VarViewId output, VarViewId x, VarViewId y);

  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarViewId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation
