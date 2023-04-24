#pragma once

#include "constraint.hpp"
#include "core/engine.hpp"
#include "core/types.hpp"
#include "variables/intVar.hpp"

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
