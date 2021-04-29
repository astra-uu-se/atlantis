#pragma once

#include <memory>
#include <vector>

#include "../core/engine.hpp"
#include "../core/types.hpp"
#include "../invariants/invariant.hpp"
#include "../variables/intVar.hpp"

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
