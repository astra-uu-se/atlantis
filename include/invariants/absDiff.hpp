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
  const VarId _x, _y, _absDiff;

 public:
  AbsDiff(VarId x, VarId y, VarId absDiff);

#ifndef CBLS_TEST
  void init(Timestamp, Engine&) final;
  void recompute(Timestamp, Engine&) final;
  void notifyIntChanged(Timestamp, Engine&, LocalId) final;
  void commit(Timestamp, Engine&) final;
  VarId getNextInput(Timestamp, Engine&) final;
  void notifyCurrentInputChanged(Timestamp, Engine&) final;
#else
  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyIntChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
  VarId getNextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
#endif
};
