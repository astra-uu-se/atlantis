#pragma once

#include <algorithm>

#include "core/types.hpp"
#include "invariants/invariant.hpp"

class Engine;

/**
 * Invariant for absDiff <== |x-y|
 *
 */

class AbsDiff : public Invariant {
 private:
  const VarId _x, _y, _absDiff;

 public:
  AbsDiff(VarId x, VarId y, VarId absDiff);

  void registerVars(Engine&) override;
  void updateBounds(Engine&, bool widenOnly = false) override;
  void recompute(Timestamp, Engine&) override;
  void notifyInputChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
  VarId nextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
};
