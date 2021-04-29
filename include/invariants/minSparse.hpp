#pragma once

#include <memory>
#include <vector>

#include "../core/engine.hpp"
#include "../variables/intVar.hpp"
#include "../invariants/invariant.hpp"
#include "../variables/savedValue.hpp"

#include "../core/types.hpp"
#include "../core/priorityList.hpp"

/**
 * Invariant for b <- min(X)
 * This version of min should be used when the domain of the variables
 * in X is large compared to the number of variables in X, and/or
 * when few elements in X are expected to take the same value.
 *
 */

class MinSparse : public Invariant {
 private:
  std::vector<VarId> m_X;
  VarId m_b;

  PriorityList m_localPriority;

 public:
  MinSparse(std::vector<VarId> X, VarId b);

  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  VarId getNextDependency(Timestamp, Engine&) override;
  void notifyCurrentDependencyChanged(Timestamp, Engine& e) override;
  void notifyIntChanged(Timestamp t, Engine& e, LocalId id) override;
  void commit(Timestamp, Engine&) override;
};
