#pragma once

#include <vector>

#include "atlantis/propagation/violationInvariants/violationInvariant.hpp"

namespace atlantis::propagation {

class GlobalCardinalityLowUp : public ViolationInvariant {
  std::vector<VarViewId> _vars;
  std::vector<Int> _lowerBounds;
  std::vector<Int> _upperBounds;
  CommittableInt _shortage;
  CommittableInt _excess;
  std::vector<CommittableInt> _counts;
  Int _offset;
  signed char increaseCount(Timestamp ts, Int value);
  signed char decreaseCount(Timestamp ts, Int value);

 public:
  GlobalCardinalityLowUp(SolverBase&, VarViewId violationId,
                         std::vector<VarViewId>&& vars,
                         const std::vector<Int>& cover,
                         const std::vector<Int>& bounds);

  GlobalCardinalityLowUp(SolverBase&, VarId violationId,
                         std::vector<VarViewId>&& vars,
                         const std::vector<Int>& cover,
                         const std::vector<Int>& bounds);

  GlobalCardinalityLowUp(SolverBase&, VarViewId violationId,
                         std::vector<VarViewId>&& vars,
                         const std::vector<Int>& cover,
                         const std::vector<Int>& lowerBounds,
                         const std::vector<Int>& upperBounds);

  GlobalCardinalityLowUp(SolverBase&, VarId violationId,
                         std::vector<VarViewId>&& vars,
                         const std::vector<Int>& cover,
                         const std::vector<Int>& lowerBounds,
                         const std::vector<Int>& upperBounds);

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
