#pragma once

#include <vector>

#include "atlantis/propagation/invariants/invariant.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/propagation/utils/priorityList.hpp"

namespace atlantis::propagation {

/**
 * Invariant for output <- max(varArray)
 *
 */

class Max : public Invariant {
 private:
  VarId _output;
  std::vector<VarViewId> _varArray;
  std::vector<std::pair<CommittableInt, CommittableInt>> _linkedList;
  CommittableInt _listHead;
  CommittableInt _updatedMax;  // The min value of the probe.
  Int _limit;

 public:
  explicit Max(SolverBase&, VarId output, std::vector<VarViewId>&& varArray);

  explicit Max(SolverBase&, VarViewId output,
               std::vector<VarViewId>&& varArray);

  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void close(Timestamp) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  void commit(Timestamp) override;
  VarViewId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation
