#pragma once

#include <limits>
#include <utility>
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

class Exists : public Invariant {
 private:
  const std::vector<VarId> _varArray;
  const VarId _y;

  PriorityList _localPriority;

 public:
  Exists(std::vector<VarId> varArray, VarId y);

  void registerVars(Engine&) override;
  void updateBounds(Engine&, bool widenOnly = false) override;
  void recompute(Timestamp, Engine&) override;
  void notifyInputChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
  VarId nextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
};
