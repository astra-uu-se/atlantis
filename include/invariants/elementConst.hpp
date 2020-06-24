#pragma once

#include <memory>
#include <vector>

#include "../core/engine.hpp"
#include "../core/intVar.hpp"
#include "../core/invariant.hpp"
#include "../core/tracer.hpp"
#include "../core/types.hpp"

/**
 * Invariant for b <- A[i] where A is a vector of constants.
 *
 */

class ElementConst : public Invariant {
 private:
  VarId m_i;
  std::vector<Int> m_A;
  VarId m_b;

 public:
  ElementConst(VarId i, std::vector<Int>&& A, VarId b);
  ~ElementConst() = default;
  void init(const Timestamp&, Engine&) override;
  void recompute(const Timestamp&, Engine&) override;
  void notifyIntChanged(const Timestamp& t, Engine& e, LocalId id, Int oldValue,
                        Int newValue, Int data) override;
  virtual VarId getNextDependency(const Timestamp&, Engine&) override;
  virtual void notifyCurrentDependencyChanged(const Timestamp&,
                                              Engine& e) override;
  void commit(const Timestamp&, Engine&) override;
};
