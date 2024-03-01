#pragma once

#include "atlantis/propagation/invariants/invariant.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {

/**
 * Invariant for output <- nominator div denominator (integer division)
 * If y = 0, then divides by 1 or -1 (depending on the domain of y)
 *
 */

class IntDiv : public Invariant {
 private:
  VarId _output, _nominator, _denominator;
  Int _zeroReplacement{1};

 public:
  explicit IntDiv(SolverBase&, VarId output, VarId nominator,
                  VarId denominator);
  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void close(Timestamp) override;
  void recompute(Timestamp) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
};

}  // namespace atlantis::propagation
