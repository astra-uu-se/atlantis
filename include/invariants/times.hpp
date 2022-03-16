#pragma once

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
  VarId _a;
  VarId _b;
  VarId _y;

 public:
  Times(VarId a, VarId b, VarId y) : Invariant(NULL_ID), _a(a), _b(b), _y(y) {
    _modifiedVars.reserve(1);
  }

  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  VarId nextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
  void notifyInputChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
};
