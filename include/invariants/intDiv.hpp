#pragma once

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

 public:
  IntDiv(VarId a, VarId b, VarId y) : Invariant(NULL_ID), _a(a), _b(b), _y(y) {
    _modifiedVars.reserve(1);
  }

  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  VarId getNextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
  void notifyIntChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
};
