#pragma once

#include "core/types.hpp"
#include "invariants/invariant.hpp"

class Engine;

/**
 * Invariant for absDiff <== |x-y|
 *
 */

class AbsDiff : public Invariant {
 private:
  VarId _x, _y, _absDiff;

 public:
  AbsDiff(VarId x, VarId y, VarId absDiff);
  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyIntChanged(Timestamp, Engine&, LocalId) override;
  VarId getNextParameter(Timestamp, Engine&) override;
  void notifyCurrentParameterChanged(Timestamp, Engine&) override;
  void commit(Timestamp, Engine&) override;
};
