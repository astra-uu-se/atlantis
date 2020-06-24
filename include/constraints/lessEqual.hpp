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
  //   Linear(Engine& e, std::vector<Int>&& A,
  //          std::vector<std::shared_ptr<IntVar>>&& X, std::shared_ptr<IntVar>
  //          b);

  ~LessEqual() = default;
  virtual void init(const Timestamp&, Engine&) override;
  virtual void recompute(const Timestamp&, Engine&) override;
  virtual void notifyIntChanged(const Timestamp& t, Engine& e, LocalId id,
                                Int oldValue, Int newValue, Int data) override;
  virtual void commit(const Timestamp&, Engine&) override;
  virtual VarId getNextDependency(const Timestamp&, Engine&) override;
  virtual void notifyCurrentDependencyChanged(const Timestamp&, Engine& e) override;
};
