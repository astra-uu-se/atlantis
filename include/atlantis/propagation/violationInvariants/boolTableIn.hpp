#pragma once

#include "atlantis/propagation/violationInvariants/violationInvariant.hpp"

namespace atlantis::propagation {
/**
 * This is a violation invariant.
 * The violation variable is the minimum of all output variables.
 * Therefore, this invariant should be combined with a min invariant
 */
class BoolTableIn : public ViolationInvariant {
 protected:
  std::vector<VarViewId> _varArray;
  std::vector<std::vector<bool>> _transposed;
  std::vector<CommittableInt> _rowViolations;
  std::vector<CommittableInt> _violationCounts;

 public:
  explicit BoolTableIn(SolverBase&, VarId violationId,
                        std::vector<VarViewId>&& vars, const std::vector<std::vector<bool>>& table);

  explicit BoolTableIn(SolverBase&, VarViewId violationId,
                        std::vector<VarViewId>&& vars, const std::vector<std::vector<bool>>& table);

  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarViewId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
    void commit(Timestamp) override;
};

}  // namespace atlantis::propagation
