#pragma once

#include <memory>
#include <vector>

#include "../core/constraint.hpp"
#include "../core/engine.hpp"
#include "../core/intVar.hpp"
#include "../core/tracer.hpp"
#include "../core/types.hpp"

class LessEqual : public Constraint {
 private:
  VarId m_x;
  VarId m_y;

 public:
  LessEqual(VarId violationId, VarId x, VarId y);

  void init(Timestamp, Engine&) override;
  void compute(Timestamp, Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyIntChanged(Timestamp t, Engine& e, LocalId id) override;
  void commit(Timestamp, Engine&) override;
  VarId getNextDependency(Timestamp, Engine&) override;
  void notifyCurrentDependencyChanged(Timestamp, Engine& e) override;
};
