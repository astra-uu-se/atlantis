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
  const std::vector<VarId> _varArray;
  const VarId _y;

  PriorityList _localPriority;

 public:
  MaxSparse(std::vector<VarId> varArray, VarId y);

#ifndef CBLS_TEST
  void init(Timestamp, Engine&) final;
  void recompute(Timestamp, Engine&) final;
  void notifyIntChanged(Timestamp, Engine&, LocalId) final;
  void commit(Timestamp, Engine&) final;
  VarId getNextInput(Timestamp, Engine&) final;
  void notifyCurrentInputChanged(Timestamp, Engine&) final;
#else
  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyIntChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
  VarId getNextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
#endif
};
