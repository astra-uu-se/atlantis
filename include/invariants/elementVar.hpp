#pragma once

#include <memory>
#include <vector>

#include "../core/engine.hpp"
#include "../core/intVar.hpp"
#include "../core/invariant.hpp"
#include "../core/types.hpp"
#include "../core/tracer.hpp"

/**
 * Invariant for b <- X[i] where X is a vector of VarId.
 *
 */

class ElementVar : public Invariant {
 private:
  VarId m_i;
  std::vector<VarId> m_X;
  VarId m_b;

 public:
  ElementVar(VarId i, std::vector<VarId>&& X,
         VarId b);
  ~ElementVar() = default;
  void init(const Timestamp&, Engine&) override;
  void recompute(const Timestamp&, Engine&) override;
  void notifyIntChanged(const Timestamp& t, Engine& e, LocalId id, Int oldValue,
                        Int newValue, Int data) override;
  virtual VarId getNextDependency(const Timestamp&, Engine&) override;
  virtual void notifyCurrentDependencyChanged(const Timestamp&, Engine& e) override;
  void commit(const Timestamp&, Engine&) override;
};
