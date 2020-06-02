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
  InvariantId newId = e.registerInvariant(
      *this);  // side effect: updates the id of this invariant
  e.registerDefinedVariable(newId, m_b->m_id);
  for (size_t i = 0; i < m_X.size(); i++) {
    e.registerDependency(newId, m_X[i]->m_id, LocalId(i), m_A[i]);
  }
}

void Linear::notifyIntChanged(const Timestamp& t, Engine& e,[[maybe_unused]] VarId id,
                              Int oldValue, Int newValue, Int data) {
  Int delta = newValue - oldValue;
  if (delta != 0) {
    m_b->incValue(t, delta * data);
    e.notifyMaybeChanged(m_b->m_id);
  }
}

void Linear::commit(const Timestamp& t) {
  this->m_isInvalid = false;
  m_b->commitIf(t);
}