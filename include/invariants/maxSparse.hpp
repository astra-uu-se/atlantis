#pragma once

#include <memory>
#include <vector>

#include "../core/engine.hpp"
#include "../core/intVar.hpp"
#include "../core/invariant.hpp"
#include "../core/savedValue.hpp"
#include "../core/tracer.hpp"
#include "../core/types.hpp"
#include "../core/priorityList.hpp"

/**
 * Invariant for b <- min(X)
 * This version of min should be used when the domain of the variables
 * in X is large compared to the number of variables in X, and/or
 * when few elements in X are expected to take the same value.
 *
 */

class MaxSparse : public Invariant {
 private:
  std::vector<VarId> m_X;
  VarId m_b;

  PriorityList m_localPriority;

 public:
  MaxSparse(std::vector<VarId> X, VarId b);

  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  VarId getNextDependency(Timestamp, Engine&) override;
  void notifyCurrentDependencyChanged(Timestamp, Engine& e) override;
  void notifyIntChanged(Timestamp t, Engine& e, LocalId id) override;
  void commit(Timestamp, Engine&) override;
};
