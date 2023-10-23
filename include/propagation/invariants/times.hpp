#pragma once

#include <algorithm>
#include <vector>

#include "types.hpp"
#include "propagation/engine.hpp"
#include "propagation/invariants/invariant.hpp"

namespace atlantis::propagation {

class Engine;

/**
 * Invariant for output <- x * y
 *
 */

class Times : public Invariant {
 private:
  const VarId _output, _x, _y;

 public:
  explicit Times(Engine&, VarId output, VarId x, VarId y);
  void registerVars() override;
  void updateBounds(bool widenOnly = false) override;
  void recompute(Timestamp) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
};

}  // namespace atlantis::propagation