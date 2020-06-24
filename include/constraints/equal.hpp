#pragma once

#include <memory>
#include <vector>

#include "../core/constraint.hpp"
#include "../core/engine.hpp"
#include "../core/intVar.hpp"
#include "../core/tracer.hpp"
#include "../core/types.hpp"

class Equal : public Constraint {
 private:
  VarId m_x;
  VarId m_y;

 public:
  Equal(VarId violationId, VarId x, VarId y);
  
  ~Equal() = default;
  virtual void init(const Timestamp&, Engine&) override;
  virtual void recompute(const Timestamp&, Engine&) override;
  virtual void notifyIntChanged(const Timestamp& t, Engine& e, LocalId id,
                                Int oldValue, Int newValue, Int data) override;
  virtual void commit(const Timestamp&, Engine&) override;
  virtual VarId getNextDependency(const Timestamp&) override;
  virtual void notifyCurrentDependencyChanged(const Timestamp&, Engine& e) override;
};
