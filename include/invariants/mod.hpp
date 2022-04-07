#pragma once

#include "core/types.hpp"
#include "invariants/invariant.hpp"

/**
 * Invariant for y <- a % b (integer division)
 *
 */
class Mod : public Invariant {
 private:
  VarId _a, _b, _y;
  Int _zeroReplacement{1};

 public:
  Mod(VarId a, VarId b, VarId y);
  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyInputChanged(Timestamp, Engine&, LocalId) override;
  VarId nextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
  void commit(Timestamp, Engine&) override;
};