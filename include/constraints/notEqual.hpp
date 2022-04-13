#pragma once

#include <algorithm>
#include <functional>

#include "constraint.hpp"
#include "core/types.hpp"
#include "variables/intVar.hpp"

class Engine;
class NotEqual : public Constraint {
 private:
  const VarId _x;
  const VarId _y;

 public:
  NotEqual(VarId violationId, VarId x, VarId y);

  void registerVars(Engine&) override;
  void updateBounds(Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyInputChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
  VarId nextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
};
