#pragma once

#include <memory>
#include <vector>

#include "../core/engine.hpp"
#include "../core/intVar.hpp"
#include "../core/invariant.hpp"
#include "../core/types.hpp"

/**
 * Invariant for b <- sum(A_i*X_i)
 *
 */

class Linear : public Invariant {
 private:
  std::vector<Int> m_A;
  std::vector<std::shared_ptr<IntVar>> m_X;
  std::shared_ptr<IntVar> m_b;

 public:
  Linear(std::vector<Int>&& A, std::vector<std::shared_ptr<IntVar>>&& X,
         std::shared_ptr<IntVar> b);
  Linear(Engine& e, std::vector<Int>&& A,
         std::vector<std::shared_ptr<IntVar>>&& X, std::shared_ptr<IntVar> b);

  ~Linear();
  void init(Engine& e) override;
  void notifyIntChanged(const Timestamp& t, Engine& e, LocalId id, Int oldValue,
                        Int newValue, Int data) override;
  void commit(const Timestamp& t) override;
};
