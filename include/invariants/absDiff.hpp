#pragma once

#include "core/types.hpp"
#include "invariants/invariant.hpp"

class Engine;

/**
 * Invariant for c <== |a-b|
 *
 */

class AbsDiff : public Invariant {
 private:
  VarId _a, _b, _c;

 public:
  AbsDiff(VarId a, VarId b, VarId c);
  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyIntChanged(Timestamp, Engine&, LocalId) override;
  VarId getNextParameter(Timestamp, Engine&) override;
  void notifyCurrentParameterChanged(Timestamp, Engine&) override;
  void commit(Timestamp, Engine&) override;
};
