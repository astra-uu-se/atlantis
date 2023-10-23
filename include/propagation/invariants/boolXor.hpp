#pragma once

#include <algorithm>
#include <functional>

#include "types.hpp"
#include "propagation/engine.hpp"
#include "propagation/invariants/invariant.hpp"
#include "propagation/variables/intVar.hpp"

namespace atlantis::propagation {

/**
 * Invariant for output <- xor(x, y) [bool(x) != bool(y)]
 *
 */

class BoolXor : public Invariant {
 private:
  const VarId _output, _x, _y;

 public:
  explicit BoolXor(Engine&, VarId output, VarId x, VarId y);
  void registerVars() override;
  void updateBounds(bool widenOnly = false) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation