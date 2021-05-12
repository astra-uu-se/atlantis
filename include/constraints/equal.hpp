#pragma once

#include "constraint.hpp"
#include "core/types.hpp"
#include "variables/intVar.hpp"

class Engine;
class Equal : public Constraint {
 private:
  VarId _x;
  VarId _y;

 public:
  Equal(VarId violationId, VarId x, VarId y);

  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyIntChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
  VarId getNextParameter(Timestamp, Engine&) override;
  void notifyCurrentParameterChanged(Timestamp, Engine&) override;
};
