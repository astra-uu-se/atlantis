#pragma once

#include <vector>

#include "atlantis/propagation/invariants/invariant.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/propagation/utils/priorityList.hpp"

namespace atlantis::propagation {

/**
 * Invariant for output <- min(varArray)
 *
 */

class MinSparse : public Invariant {
 private:
  VarId _output;
  std::vector<VarViewId> _varArray;
  PriorityList _localPriority;

 public:
  explicit MinSparse(SolverBase&, VarId output,
                     std::vector<VarViewId>&& varArray);

  explicit MinSparse(SolverBase&, VarViewId output,
                     std::vector<VarViewId>&& varArray);

  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  void commit(Timestamp) override;
  VarViewId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation
