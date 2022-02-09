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

class MinSparse : public Invariant {
 private:
  const std::vector<VarId> _varArray;
  const VarId _y;

  PriorityList _localPriority;

 public:
  MinSparse(std::vector<VarId> varArray, VarId y);

#ifndef NDEBUG
  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyIntChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
  VarId getNextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
#else
  void init(Timestamp, Engine&) final override;
  void recompute(Timestamp, Engine&) final override;
  void notifyIntChanged(Timestamp, Engine&, LocalId) final override;
  void commit(Timestamp, Engine&) final override;
  VarId getNextInput(Timestamp, Engine&) final override;
  void notifyCurrentInputChanged(Timestamp, Engine&) final override;
#endif
};
