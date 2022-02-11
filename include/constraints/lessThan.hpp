#pragma once

#include "constraint.hpp"
#include "core/types.hpp"
#include "variables/intVar.hpp"

class Engine;

class LessThan : public Constraint {
 private:
  const VarId _x;
  const VarId _y;

 public:
  LessThan(VarId violationId, VarId x, VarId y);

#ifndef CBLS_TEST
  void init(Timestamp, Engine&) final;
  void recompute(Timestamp, Engine&) final;
  void notifyIntChanged(Timestamp, Engine&, LocalId) final;
  void commit(Timestamp, Engine&) final;
  VarId getNextInput(Timestamp, Engine&) final;
  void notifyCurrentInputChanged(Timestamp, Engine&) final;
#else
  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyIntChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
  VarId getNextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
#endif
};
