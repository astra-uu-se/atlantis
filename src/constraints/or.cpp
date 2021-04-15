#include "constraints/or.hpp"
#include <utility>

// Temporary naive implementation that recomputes min(X).
// Currently used in VesselLoading benchmark.
// Use invariant/MinSparse instead once it has been tested

Or::Or(VarId violationId, std::vector<VarId> X)
    : Constraint(NULL_ID, violationId),
      m_X(std::move(X)) {
  m_modifiedVars.reserve(m_X.size());
}

void Or::init(Timestamp, Engine& e) {
  assert(m_id != NULL_ID);
  registerDefinedVariable(e, m_violationId);
  for (size_t i = 0; i < m_X.size(); ++i) {
    e.registerInvariantDependsOnVar(m_id, m_X[i], LocalId(0));
  }
}

void Or::recompute(Timestamp t, Engine& e) {
  Int min = e.getValue(t, m_X.front());
  for (size_t i = 1; i < m_X.size(); ++i) {
    min = std::min(min, e.getValue(t, m_X[i]));
  }
  updateValue(t, e, m_violationId, min);
}

// Temporary
void Or::notifyIntChanged(Timestamp t, Engine& e, LocalId i) {
  auto newValue = e.getValue(t, m_X[i]);
  if (newValue < e.getValue(t, m_violationId)) {
    updateValue(t, e, m_violationId, newValue);
  } else {
    recompute(t, e);
  }
}

VarId Or::getNextDependency(Timestamp t, Engine&) {
  m_state.incValue(t, 1);
  if (static_cast<size_t>(m_state.getValue(t)) == m_X.size()) {
    return NULL_ID;  // Done
  } else {
    return m_X.at(m_state.getValue(t));
  }
}

void Or::notifyCurrentDependencyChanged(Timestamp t, Engine& e) {
  assert(m_state.getValue(t) != -1);
  Int idx = m_state.getValue(t);
  notifyIntChanged(t, e, idx);
}

void Or::commit(Timestamp t, Engine& e) { Invariant::commit(t, e); }
