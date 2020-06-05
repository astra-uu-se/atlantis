#include "invariants/linear.hpp"

#include <vector>

Linear::Linear(std::vector<Int>&& A, std::vector<std::shared_ptr<IntVar>>&& X,
               std::shared_ptr<IntVar> b)
    : Invariant(Engine::NULL_ID),
      m_A(std::move(A)),
      m_X(std::move(X)),
      m_b(b) {}

Linear::Linear(Engine& e, std::vector<Int>&& A,
               std::vector<std::shared_ptr<IntVar>>&& X,
               std::shared_ptr<IntVar> b)
    : Invariant(Engine::NULL_ID), m_A(std::move(A)), m_X(std::move(X)), m_b(b) {
  init(e);
}

void Linear::init(Engine& e) {
  // precondition: this invariant must be registered with the engine before it is initialised.
  assert(m_id != Engine::NULL_ID);
  e.registerDefinedVariable(m_id, m_b->m_id);
  for (size_t i = 0; i < m_X.size(); i++) {
    e.registerInvariantDependency(m_id, m_X[i]->m_id, LocalId(i), m_A[i]);
  }

  Int sum = 0;
  for (size_t i = 0; i < m_X.size(); i++) {
    sum += m_A[i] * m_X[i]->getCommittedValue();
  }
  m_b->commitValue(sum);
  e.notifyMaybeChanged(m_b->m_id);
  this->validate();
}

void Linear::notifyIntChanged(const Timestamp& t, Engine& e,
                              [[maybe_unused]] LocalId id, Int oldValue,
                              Int newValue, Int data) {
  assert(newValue != oldValue);  // precondition
  m_b->incValue(t, (newValue - oldValue) * data);
  e.notifyMaybeChanged(m_b->m_id);
}

void Linear::commit(const Timestamp& t) {
  this->validate();
  m_b->commitIf(t);
}