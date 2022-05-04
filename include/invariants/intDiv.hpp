#pragma once

#include <algorithm>
#include <limits>
#include <vector>

#include "core/types.hpp"
#include "invariants/invariant.hpp"

class Engine;

/**
 * Invariant for y <- a div c (integer division)
 *
 */

class IntDiv : public Invariant {
 private:
  VarId _a;
  VarId _b;
  VarId _y;
  Int _zeroReplacement{1};

 public:
  IntDiv(VarId a, VarId b, VarId y);
  void registerVars(Engine&) override;
  void updateBounds(Engine&, bool widenOnly = false) override;
  void close(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  VarId nextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
  void notifyInputChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
};
