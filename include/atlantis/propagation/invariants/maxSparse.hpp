#pragma once

#include <limits>
#include <vector>

#include "atlantis/propagation/invariants/invariant.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/utils/priorityList.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {

/**
 * Invariant for output <- max(varArray)
 *
 */

class MaxSparse : public Invariant {
 private:
  VarId _output;
  std::vector<VarId> _varArray;

  PriorityList _localPriority;

 public:
  explicit MaxSparse(SolverBase&, VarId output, std::vector<VarId>&& varArray);

  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  void commit(Timestamp) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation
