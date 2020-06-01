#pragma once
#include <vector>

#include "../core/intVar.hpp"
#include "../core/invariant.hpp"
#include "../core/types.hpp"
#include "../core/engine.hpp"

class Linear : public Invariant {
 private:
  std::vector<Int> m_A;
  std::vector<std::weak_ptr<IntVar>> m_X;
  Int m_c;

 public:
  Linear(Engine& e, Id id, std::vector<Int>&& A, std::vector<std::weak_ptr<IntVar>>&& X,
         Int c);
  ~Linear();
};
