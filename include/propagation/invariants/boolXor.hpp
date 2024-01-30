#pragma once

#include <algorithm>
#include <functional>

#include "types.hpp"
#include "propagation/solver.hpp"
#include "propagation/invariants/invariant.hpp"
#include "propagation/variables/intVar.hpp"

namespace atlantis::propagation {

/**
 * Invariant for output <- xor(x, y) [bool(x) != bool(y)]
 *
 */

class BoolXor : public Invariant {
 private:
  VarId _output, _x, _y;

 public:
  explicit BoolXor(SolverBase&, VarId output, VarId x, VarId y);
  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation