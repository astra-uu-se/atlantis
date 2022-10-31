#pragma once

#include <algorithm>
#include <cmath>
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
  explicit Pow(Engine&, VarId output, VarId x, VarId y);
  void registerVars() override;
  void updateBounds(bool widenOnly = false) override;
  void recompute(Timestamp) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  void commit(Timestamp) override;
};
