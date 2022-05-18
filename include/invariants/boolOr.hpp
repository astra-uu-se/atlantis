#pragma once

#include "core/types.hpp"
#include "invariants/invariant.hpp"
#include "variables/intVar.hpp"

class Engine;

class BoolOr : public Invariant {
 private:
  const VarId _x;
  const VarId _y;
  const VarId _output;

 public:
  explicit BoolOr(VarId x, VarId y, VarId output);

  void registerVars(Engine&) override;
  void updateBounds(Engine&, bool widenOnly = false) override;
  void recompute(Timestamp, Engine&) override;
  void notifyInputChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
  VarId nextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
};
