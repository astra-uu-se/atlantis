#pragma once

#include "atlantis/propagation/invariants/invariant.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {

/**
 * Invariant for output <- x % y (integer division)
 *
 */
class Mod : public Invariant {
 private:
  VarId _output;
  VarViewId _numerator, _denominator;

 public:
  explicit Mod(SolverBase&, VarId output, VarViewId numerator,
               VarViewId denominator);

  explicit Mod(SolverBase&, VarViewId output, VarViewId numerator,
               VarViewId denominator);
  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void close(Timestamp) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarViewId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation
