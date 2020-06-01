#include "invariants/linear.hpp"

Linear::Linear(Engine& e, Id id, std::vector<Int>&& A,
               std::vector<std::weak_ptr<IntVar>>&& X, Int c)
    : Invariant(id), m_A(std::move(A)), m_X(std::move(X)), m_c(c) {
  for (auto x : m_X) {
    // if (auto xPtr = x.lock()) {
    //   e.registerDependency(m_id, xPtr->m_id, []() { return i; });
    // }
  }
}