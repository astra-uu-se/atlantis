#pragma once

#include <vector>
#include <memory>

#include "../core/intVar.hpp"
#include "../core/invariant.hpp"
#include "../core/types.hpp"
#include "../core/engine.hpp"

class Linear : public Invariant {
 private:
  std::vector<Int> m_A;
  std::vector<std::shared_ptr<IntVar>> m_X;

 public:
  Linear(Engine& e, Id id, std::vector<Int>&& A,
         std::vector<std::shared_ptr<IntVar>>&& X);
  ~Linear();

  void notifyIntChanged(Engine& e, Id id, Int oldValue, Int newValue,
                        Int data) override;
};
