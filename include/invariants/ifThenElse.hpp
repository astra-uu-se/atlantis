#pragma once

#include <memory>
#include <vector>

#include "../core/engine.hpp"
#include "../core/intVar.hpp"
#include "../core/invariant.hpp"
#include "../core/tracer.hpp"
#include "../core/types.hpp"

/**
 * Invariant for z <- x if b = 0 else y.
 *
 */

class IfThenElse : public Invariant {
 private:
  std::vector<VarId> m_xy;
  VarId m_b, m_z;

 public:
  IfThenElse(VarId x, VarId y, VarId b, VarId z);
  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyIntChanged(Timestamp t, Engine& e, LocalId id) override;
  VarId getNextDependency(Timestamp, Engine&) override;
  void notifyCurrentDependencyChanged(Timestamp, Engine& e) override;
  void commit(Timestamp, Engine&) override;
};