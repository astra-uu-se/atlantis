#pragma once

#include <algorithm>
#include <limits>
#include <vector>

#include "types.hpp"
#include "propagation/engine.hpp"
#include "propagation/invariants/invariant.hpp"

namespace atlantis::propagation {

class Engine;

/**
 * Invariant for output <- x div y (integer division)
 * If y = 0, then divides by 1 or -1 (depending on the domain of y)
 *
 */

class IntDiv : public Invariant {
 private:
  VarId _output, _x, _y;
  Int _zeroReplacement{1};

 public:
  explicit IntDiv(Engine&, VarId output, VarId x, VarId y);
  void registerVars() override;
  void updateBounds(bool widenOnly = false) override;
  void close(Timestamp) override;
  void recompute(Timestamp) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
};

}  // namespace atlantis::propagation