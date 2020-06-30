#pragma once

#include <memory>
#include <vector>

#include "../core/engine.hpp"
#include "../core/intVar.hpp"
#include "../core/invariant.hpp"
#include "../core/tracer.hpp"
#include "../core/types.hpp"

/**
 * Invariant for c <- |a-b|
 *
 */

class AbsDiff : public Invariant {
 private:
  VarId m_a, m_b, m_c;

 public:
  AbsDiff(VarId a, VarId b, VarId c);
  ~AbsDiff() = default;
  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyIntChanged(Timestamp t, Engine& e, LocalId id, Int oldValue,
                        Int newValue, Int data) override;
  virtual VarId getNextDependency(Timestamp, Engine&) override;
  virtual void notifyCurrentDependencyChanged(Timestamp, Engine& e) override;
  void commit(Timestamp, Engine&) override;
};
