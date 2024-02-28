#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include "atlantis/propagation/invariants/invariant.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {

/**
 * Invariant for y <- a ^ b
 *
 */
class Pow : public Invariant {
 private:
  VarId _output, _base, _exponent;
  Int _zeroReplacement{1};

 public:
  explicit Pow(SolverBase&, VarId output, VarId base, VarId exponent);
  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void recompute(Timestamp) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
};

}  // namespace atlantis::propagation
