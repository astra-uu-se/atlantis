#pragma once

#include <memory>
#include <vector>

#include "../core/types.hpp"
#include "../variables/intVar.hpp"
#include "constraint.hpp"

class Engine;

class LessThan : public Constraint {
 private:
  VarId m_x;
  VarId m_y;

 public:
  LessThan(VarId violationId, VarId x, VarId y);

  void init(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyIntChanged(Timestamp t, Engine& e, LocalId id) override;
  void commit(Timestamp, Engine&) override;
  VarId getNextDependency(Timestamp, Engine&) override;
  void notifyCurrentDependencyChanged(Timestamp, Engine& e) override;
};
