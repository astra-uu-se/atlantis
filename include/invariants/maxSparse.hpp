#pragma once

#include <vector>

#include "core/engine.hpp"
#include "core/types.hpp"
#include "invariants/invariant.hpp"
#include "utils/priorityList.hpp"

/**
 * Invariant for b <- min(X)
 * This version of min should be used when the domain of the variables
 * in X is large compared to the number of variables in X, and/or
 * when few elements in X are expected to take the same value.
 *
 */

class MaxSparse : public Invariant {
 private:
  std::vector<VarId> _X;
  VarId _b;

  PriorityList _localPriority;

 public:
  MaxSparse(std::vector<VarId> X, VarId b);

  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  VarId getNextDependency(Timestamp, Engine&) override;
  void notifyCurrentDependencyChanged(Timestamp, Engine&) override;
  void notifyIntChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
};
