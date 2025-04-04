#pragma once

#include "atlantis/propagation/violationInvariants/violationInvariant.hpp"

namespace atlantis::propagation {

class PowDomain : public ViolationInvariant {
  VarViewId _x, _y;

 public:
  explicit PowDomain(SolverBase&, VarId violationId, VarViewId x, VarViewId y);

  explicit PowDomain(SolverBase&, VarViewId violationId, VarViewId x,
                     VarViewId y);

  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarViewId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;

  static bool shouldPost(const SolverBase&, VarViewId x, VarViewId y);
};

}  // namespace atlantis::propagation
