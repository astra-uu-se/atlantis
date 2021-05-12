#pragma once

#include <vector>

#include "core/engine.hpp"
#include "core/types.hpp"
#include "invariants/invariant.hpp"
#include "utils/priorityList.hpp"

/**
 * Invariant for y <- min(varArray)
 * This version of min should be used when the domain of the variables
 * in varArray is large compared to the number of variables in varArray, and/or
 * when few elements in varArray are expected to take the same value.
 *
 */

class MaxSparse : public Invariant {
 private:
  std::vector<VarId> _varArray;
  VarId _y;

  PriorityList _localPriority;

 public:
  MaxSparse(std::vector<VarId> varArray, VarId y);

  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  VarId getNextParameter(Timestamp, Engine&) override;
  void notifyCurrentParameterChanged(Timestamp, Engine&) override;
  void notifyIntChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
};
