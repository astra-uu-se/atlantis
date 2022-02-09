#pragma once

#include "constraint.hpp"
#include "core/types.hpp"
#include "variables/intVar.hpp"

class Engine;

class Equal : public Constraint {
 private:
  const VarId _x;
  const VarId _y;

 public:
  Equal(VarId violationId, VarId x, VarId y);

#ifndef NDEBUG
  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyIntChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
  VarId getNextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
#else
  void init(Timestamp, Engine&) final override;
  void recompute(Timestamp, Engine&) final override;
  void notifyIntChanged(Timestamp, Engine&, LocalId) final override;
  void commit(Timestamp, Engine&) final override;
  VarId getNextInput(Timestamp, Engine&) final override;
  void notifyCurrentInputChanged(Timestamp, Engine&) final override;
#endif
};
