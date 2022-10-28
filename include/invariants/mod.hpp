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
  explicit Mod(Engine&, VarId output, VarId x, VarId y);
  void registerVars() override;
  void updateBounds(bool widenOnly = false) override;
  void close(Timestamp) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
  void commit(Timestamp) override;
};