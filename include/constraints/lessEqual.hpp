#pragma once

#include <memory>
#include <vector>

#include "../core/engine.hpp"
#include "../core/intVar.hpp"
#include "../core/constraint.hpp"
#include "../core/types.hpp"
#include "../core/tracer.hpp"

class LessEqual : public Constraint {
 private:
  Int m_a;
  VarId m_x;
  Int m_b;
  VarId m_y;

 public:
  LessEqual(VarId violationId, VarId x, VarId y);
//   Linear(Engine& e, std::vector<Int>&& A,
//          std::vector<std::shared_ptr<IntVar>>&& X, std::shared_ptr<IntVar> b);

  ~LessEqual() = default;
  void init(const Timestamp&, Engine&) override;
  void recompute(const Timestamp&, Engine&) override;
  void notifyIntChanged(const Timestamp& t, Engine& e, LocalId id, Int oldValue,
                        Int newValue, Int data) override;
  void commit(const Timestamp&, Engine&) override;
};
