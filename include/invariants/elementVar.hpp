#pragma once

#include <memory>
#include <vector>

#include "../core/engine.hpp"
#include "../core/intVar.hpp"
#include "../core/invariant.hpp"
#include "../core/tracer.hpp"
#include "../core/types.hpp"

/**
 * Invariant for b <- X[i] where X is a vector of VarId.
 *
 */

class ElementVar : public Invariant {
 private:
  VarId m_i;
  std::vector<VarId> m_X;
  VarId m_b;

 public:
  ElementVar(VarId i, std::vector<VarId> X, VarId b);
  void init(Timestamp, Engine&) override;
  void compute(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyIntChanged(Timestamp t, Engine& e, LocalId id) override;
  VarId getNextDependency(Timestamp, Engine&) override;
  void notifyCurrentDependencyChanged(Timestamp, Engine& e) override;
  void commit(Timestamp, Engine&) override;
};
