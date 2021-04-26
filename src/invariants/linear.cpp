#include "invariants/linear.hpp"

#include <utility>

Linear::Linear(std::vector<Int> A, std::vector<VarId> X, VarId b)
    : Invariant(NULL_ID),
      m_A(std::move(A)),
      m_X(std::move(X)),
      m_localX(),
      m_b(b) {
  m_localX.reserve(m_X.size());
  m_modifiedVars.reserve(m_X.size());
}

void Linear::init(Timestamp t, Engine& e) {
  // precondition: this invariant must be registered with the engine before it
  // is initialised.
  assert(m_id != NULL_ID);

  registerDefinedVariable(e, m_b);
  for (size_t i = 0; i < m_X.size(); ++i) {
    e.registerInvariantDependsOnVar(m_id, m_X[i], LocalId(i));
    m_localX.emplace_back(t, e.getCommittedValue(m_X[i]));
  }
}

void Linear::recompute(Timestamp t, Engine& e) {
  Int sum = 0;
  for (size_t i = 0; i < m_X.size(); ++i) {
    sum += m_A[i] * e.getValue(t, m_X[i]);
    m_localX.at(i).commitValue(e.getCommittedValue(m_X[i]));
    m_localX.at(i).setValue(t, e.getValue(t, m_X[i]));
  }
  updateValue(t, e, m_b, sum);
}

void Linear::notifyIntChanged(Timestamp t, Engine& e, LocalId i) {
  auto newValue = e.getValue(t, m_X[i]);
  incValue(t, e, m_b, (newValue - m_localX.at(i).getValue(t)) * m_A[i]);
  m_localX.at(i).setValue(t, newValue);
}

VarId Linear::getNextDependency(Timestamp t, Engine&) {
  m_state.incValue(t, 1);
  if (static_cast<size_t>(m_state.getValue(t)) == m_X.size()) {
    return NULL_ID;  // Done
  } else {
    return m_X.at(m_state.getValue(t));
  }
}

void Linear::notifyCurrentDependencyChanged(Timestamp t, Engine& e) {
  assert(m_state.getValue(t) != -1);
  notifyIntChanged(t, e, m_state.getValue(t));
}

void Linear::commit(Timestamp t, Engine& e) {
  Invariant::commit(t, e);
  for (size_t i = 0; i < m_X.size(); ++i) {
    m_localX.at(i).commitIf(t);
  }
}
