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
  VarId m_a, m_b, m_c;

 public:
  AbsDiff(VarId a, VarId b, VarId c);
  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyIntChanged(Timestamp t, Engine& e, LocalId id) override;
  VarId getNextDependency(Timestamp, Engine&) override;
  void notifyCurrentDependencyChanged(Timestamp, Engine& e) override;
  void commit(Timestamp, Engine&) override;
};
