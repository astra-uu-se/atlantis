#pragma once

#include <vector>

#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/propagation/variables/committableInt.hpp"
#include "atlantis/propagation/violationInvariants/violationInvariant.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {

class BoolAllEqual : public ViolationInvariant {
 protected:
  std::vector<VarViewId> _vars;
  CommittableInt _numTrue;
  std::vector<CommittableInt> _varNotified;

 public:
  explicit BoolAllEqual(SolverBase&, VarId violationId,
                        std::vector<VarViewId>&& vars);

  explicit BoolAllEqual(SolverBase&, VarViewId violationId,
                        std::vector<VarViewId>&& vars);

  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  void commit(Timestamp) override;
  VarViewId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation
