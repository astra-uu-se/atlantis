#pragma once
#include <cmath>

#include "core/types.hpp"
#include "invariants/invariant.hpp"

/**
 * Invariant for output <- max(x, y)
 *
 */
class BinaryMax : public Invariant {
 private:
  const VarId _output, _x, _y;

 public:
  explicit BinaryMax(VarId output, VarId x, VarId y);
  void registerVars(Engine&) override;
  void updateBounds(Engine&, bool widenOnly = false) override;
  void recompute(Timestamp, Engine&) override;
  void notifyInputChanged(Timestamp, Engine&, LocalId) override;
  VarId nextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
  void commit(Timestamp, Engine&) override;
};