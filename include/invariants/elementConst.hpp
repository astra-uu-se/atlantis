#pragma once

#include <memory>
#include <vector>

#include "../core/engine.hpp"
#include "../variables/intVar.hpp"
#include "../invariants/invariant.hpp"

#include "../core/types.hpp"

/**
 * Invariant for b <- A[i] where A is a vector of constants.
 *
 */

class ElementConst : public Invariant {
 private:
  VarId m_i;
  std::vector<Int> m_A;
  VarId m_b;

 public:
  ElementConst(VarId i, std::vector<Int> A, VarId b);
  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyIntChanged(Timestamp t, Engine& e, LocalId id) override;
  VarId getNextDependency(Timestamp, Engine&) override;
  void notifyCurrentDependencyChanged(Timestamp, Engine& e) override;
  void commit(Timestamp, Engine&) override;
};
