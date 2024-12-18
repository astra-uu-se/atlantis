#pragma once

#include "atlantis/propagation/violationInvariants/violationInvariant.hpp"

namespace atlantis::propagation {

class BoolLessEqual : public ViolationInvariant {
  VarViewId _x, _y;

 public:
  explicit BoolLessEqual(SolverBase&, VarId violationId, VarViewId x,
                         VarViewId y);

  explicit BoolLessEqual(SolverBase&, VarViewId violationId, VarViewId x,
                         VarViewId y);

  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarViewId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation
