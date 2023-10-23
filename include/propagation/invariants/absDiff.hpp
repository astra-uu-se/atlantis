#pragma once

#include <algorithm>

#include "types.hpp"
#include "propagation/engine.hpp"
#include "propagation/invariants/invariant.hpp"

namespace atlantis::propagation {

class Engine;

/**
 * Invariant for output <== |x - y|
 *
 */

class AbsDiff : public Invariant {
 private:
  const VarId _output, _x, _y;

 public:
  explicit AbsDiff(Engine&, VarId output, VarId x, VarId y);

  void registerVars() override;
  void updateBounds(bool widenOnly = false) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation