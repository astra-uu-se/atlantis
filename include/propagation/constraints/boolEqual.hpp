#pragma once

#include "constraint.hpp"
#include "types.hpp"
#include "propagation/engine.hpp"
#include "propagation/variables/intVar.hpp"

namespace atlantis::propagation {

class BoolEqual : public Constraint {
 private:
  const VarId _x, _y;

 public:
  explicit BoolEqual(Engine&, VarId violationId, VarId x, VarId y);

  void registerVars() override;
  void updateBounds(bool widenOnly = false) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation