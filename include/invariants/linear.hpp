#pragma once

#include <memory>
#include <vector>

#include "../core/engine.hpp"
#include "../core/intVar.hpp"
#include "../core/invariant.hpp"
#include "../core/tracer.hpp"
#include "../core/types.hpp"

/**
 * Invariant for b <- sum(A_i*X_i)
 *
 */

class Linear : public Invariant {
 private:
  std::vector<Int> m_A;
  std::vector<VarId> m_X;
  VarId m_b;

 public:
  Linear(std::vector<Int>&& A, std::vector<VarId>&& X, VarId b);
  //   Linear(Engine& e, std::vector<Int>&& A,
  //          std::vector<std::shared_ptr<IntVar>>&& X, std::shared_ptr<IntVar>
  //          b);

  ~Linear() = default;
  virtual void init(const Timestamp&, Engine&) override;
  virtual void recompute(const Timestamp&, Engine&) override;
  virtual VarId getNextDependency(const Timestamp&, Engine&) override;
  virtual void notifyCurrentDependencyChanged(const Timestamp&, Engine& e) override;
  virtual void notifyIntChanged(const Timestamp& t, Engine& e, LocalId id,
                                Int oldValue, Int newValue, Int data) override;
  virtual void commit(Timestamp, Engine&) override;
};
