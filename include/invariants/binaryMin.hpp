#pragma once
#include <cmath>

#include "core/types.hpp"
#include "invariants/invariant.hpp"

/**
 * Invariant for y <- min(a, b)
 *
 */
class BinaryMin : public Invariant {
 private:
  const VarId _a;
  const VarId _b;
  const VarId _y;

 public:
  explicit BinaryMin(VarId a, VarId b, VarId y);
  void registerVars(Engine&) override;
  void updateBounds(Engine&, bool widenOnly = false) override;
  void recompute(Timestamp, Engine&) override;
  void notifyInputChanged(Timestamp, Engine&, LocalId) override;
  VarId nextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
  void commit(Timestamp, Engine&) override;
};