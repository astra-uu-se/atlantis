#pragma once
#include <cmath>

#include "core/types.hpp"
#include "invariants/invariant.hpp"

/**
 * Invariant for output <- x % y (integer division)
 *
 */
class Mod : public Invariant {
 private:
  VarId _output, _x, _y;
  Int _zeroReplacement{1};

 public:
  explicit Mod(VarId output, VarId x, VarId y);
  void registerVars(Engine&) override;
  void updateBounds(Engine&, bool widenOnly = false) override;
  void close(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyInputChanged(Timestamp, Engine&, LocalId) override;
  VarId nextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
  void commit(Timestamp, Engine&) override;
};