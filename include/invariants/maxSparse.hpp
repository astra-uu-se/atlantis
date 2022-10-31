#pragma once

#include <limits>
#include <vector>

#include "core/engine.hpp"
#include "core/types.hpp"
#include "invariants/invariant.hpp"
#include "utils/priorityList.hpp"

/**
 * Invariant for output <- max(varArray)
 *
 */

class MaxSparse : public Invariant {
 private:
  const VarId _output;
  const std::vector<VarId> _varArray;

  PriorityList _localPriority;

 public:
  explicit MaxSparse(Engine&, VarId output, std::vector<VarId> varArray);

  void registerVars() override;
  void updateBounds(bool widenOnly = false) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  void commit(Timestamp) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};
