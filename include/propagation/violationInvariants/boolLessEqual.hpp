#pragma once

#include "propagation/violationInvariants/violationInvariant.hpp"
#include "types.hpp"
#include "propagation/solver.hpp"
#include "propagation/variables/intVar.hpp"

namespace atlantis::propagation {

class BoolLessEqual : public ViolationInvariant {
 private:
  const VarId _x, _y;

 public:
  explicit BoolLessEqual(SolverBase&, VarId violationId, VarId x, VarId y);

  void registerVars() override;
  void updateBounds(bool widenOnly = false) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation