#pragma once

#include <algorithm>
#include <limits>
#include <vector>

#include "core/types.hpp"
#include "invariants/invariant.hpp"

class Engine;

/**
 * Invariant for output <- x div y (integer division)
 * If y = 0, then divides by 1 or -1 (depending on the domain of y)
 *
 */

class IntDiv : public Invariant {
 private:
  VarId _output, _x, _y;
  Int _zeroReplacement{1};

 public:
  explicit IntDiv(VarId output, VarId x, VarId y);
  void registerVars(Engine&) override;
  void updateBounds(Engine&, bool widenOnly = false) override;
  void close(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  VarId nextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
  void notifyInputChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
};
