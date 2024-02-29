#pragma once

#include <algorithm>
#include <functional>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/variables/intVar.hpp"
#include "atlantis/propagation/violationInvariants/violationInvariant.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {

class NotEqual : public ViolationInvariant {
 private:
  VarId _x, _y;

 public:
  explicit NotEqual(SolverBase&, VarId violationId, VarId x, VarId y);

  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation
