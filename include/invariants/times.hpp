#pragma once

#include <algorithm>
#include <vector>

#include "core/types.hpp"
#include "invariants/invariant.hpp"

class Engine;

/**
 * Invariant for y <- a * b
 *
 */

class Times : public Invariant {
 private:
  const VarId _a;
  const VarId _b;
  const VarId _y;

 public:
  Times(VarId a, VarId b, VarId y) : Invariant(NULL_ID), _a(a), _b(b), _y(y) {
    _modifiedVars.reserve(1);
  }

  void registerVars(Engine&) override;
  void updateBounds(Engine&, bool widenOnly = false) override;
  void recompute(Timestamp, Engine&) override;
  VarId nextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
  void notifyInputChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
};
