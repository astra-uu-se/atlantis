#pragma once

#include "constraint.hpp"
#include "types.hpp"
#include "propagation/engine.hpp"
#include "propagation/variables/intVar.hpp"

namespace atlantis::propagation {

class PowDomain : public Constraint {
 private:
  const VarId _x, _y;

 public:
  explicit PowDomain(Engine&, VarId violationId, VarId x, VarId y);

  void registerVars() override;
  void updateBounds(bool widenOnly = false) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;

  static bool shouldPost(Engine&, VarId x, VarId y);
};

}  // namespace atlantis::propagation