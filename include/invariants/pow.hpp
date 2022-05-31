#pragma once

#include <algorithm>
#include <vector>

#include "core/types.hpp"
#include "invariants/invariant.hpp"

class Engine;

/**
 * Invariant for y <- a ^ b
 *
 */
class Pow : public Invariant {
 private:
  const VarId _output, _x, _y;
  Int _zeroReplacement{1};

 public:
  explicit Pow(VarId output, VarId x, VarId y);
  void registerVars(Engine&) override;
  void updateBounds(Engine&, bool widenOnly = false) override;
  void recompute(Timestamp, Engine&) override;
  VarId nextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
  void notifyInputChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
};
