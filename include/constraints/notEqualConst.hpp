#pragma once

#include <algorithm>
#include <functional>

#include "constraint.hpp"
#include "core/types.hpp"
#include "variables/intVar.hpp"

class Engine;
class NotEqualConst : public Constraint {
 private:
  const VarId _x;
  const Int _y;

 public:
  NotEqualConst(VarId violationId, VarId x, Int y);

  void registerVars(Engine&) override;
  void updateBounds(Engine&, bool widenOnly = false) override;
  void recompute(Timestamp, Engine&) override;
  void notifyInputChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
  VarId nextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;

  static bool shouldPost(Engine& engine, VarId x, Int y);
};
