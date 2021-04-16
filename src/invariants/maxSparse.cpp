#include "invariants/maxSparse.hpp"

MaxSparse::MaxSparse(std::vector<VarId> X, VarId b)
    : Invariant(NULL_ID), m_X(X), m_b(b), m_localPriority(X.size()) {
  m_modifiedVars.reserve(m_X.size());
}

void MaxSparse::init([[maybe_unused]] Timestamp t, Engine& e) {
  assert(!m_id.equals(NULL_ID));

  registerDefinedVariable(e, m_b);
  for (size_t i = 0; i < m_X.size(); ++i) {
    e.registerInvariantDependsOnVar(m_id, m_X[i], LocalId(i));
  }
}

void MaxSparse::recompute(Timestamp t, Engine& e) {
  for (size_t i = 0; i < m_X.size(); ++i) {
    m_localPriority.updatePriority(t, i, e.getValue(t, m_X[i]));
  }
  updateValue(t, e, m_b, m_localPriority.getMaxPriority(t));
}

void MaxSparse::notifyIntChanged(Timestamp t, Engine& e, LocalId i) {
  auto newValue = e.getValue(t, m_X[i]);
  m_localPriority.updatePriority(t, i, newValue);
  updateValue(t, e, m_b, m_localPriority.getMaxPriority(t));
}

VarId MaxSparse::getNextDependency(Timestamp t, Engine&) {
  m_state.incValue(t, 1);
  if (static_cast<size_t>(m_state.getValue(t)) == m_X.size()) {
    return NULL_ID;  // Done
  } else {
    return m_X.at(m_state.getValue(t));
  }
}

void MaxSparse::notifyCurrentDependencyChanged(Timestamp t, Engine& e) {
  notifyIntChanged(t, e, m_state.getValue(t));
}

void MaxSparse::commit(Timestamp t, Engine& e) {
  Invariant::commit(t, e);
  m_localPriority.commitIf(t);
}