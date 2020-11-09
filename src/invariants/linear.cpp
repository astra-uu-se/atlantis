#include "invariants/linear.hpp"

#include <utility>

// TODO: invariant should take its true id in the constructor.

Linear::Linear(std::vector<Int> A, std::vector<VarId> X, VarId b)
    : Invariant(NULL_ID),
      m_A(std::move(A)),
      m_X(std::move(X)),
      m_localX(),
      m_b(b) {
  m_localX.reserve(m_X.size());
  m_modifiedVariables.resize(m_X.size(), false);
}

void Linear::init(Timestamp t, Engine& e) {
  // precondition: this invariant must be registered with the engine before it
  // is initialised.
  assert(m_id != NULL_ID);

  e.registerDefinedVariable(m_b, m_id);
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
  e.updateValue(t, m_b, sum);
  // m_state.setValue(t, m_X.size());  // Not clear if we actually need to reset
  // this.
  m_modifiedVariables.assign(m_modifiedVariables.size(), false);
}

void Linear::compute(Timestamp t, Engine& e) {
  Int inc = 0;
  for (size_t i = 0; i < m_modifiedVariables.size(); ++i) {
    if (!m_modifiedVariables[i]) {
      continue;
    }
    inc += m_A[i] * (e.getValue(t, m_X.at(i)) - e.getCommittedValue(m_X.at(i)));
  }
  m_modifiedVariables.assign(m_modifiedVariables.size(), false);
  e.incValue(t, m_b, inc);
}

void Linear::notifyIntChanged(Timestamp t, Engine& e, LocalId i) {
  m_modifiedVariables[i] = true;
  e.notifyMaybeChanged(t, m_b);
  //e.incValue(t, m_b, (newValue - m_localX.at(i).getValue(t)) * m_A[i]);
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
  Int idx = m_state.getValue(t);
  // Int delta = e.getValue(t, m_X.at(idx)) - e.getCommittedValue(m_X.at(idx));
  Int delta = e.getValue(t, m_X.at(idx)) - m_localX.at(idx).getValue(t);
  if (delta == 0) {
    return;
  }
  e.incValue(t, m_b, delta * m_A.at(idx));
  m_localX.at(idx).setValue(t, e.getValue(t, m_X.at(idx)));
}

void Linear::commit(Timestamp t, Engine& e) {
  Invariant::commit(t, e);
  for (size_t i = 0; i < m_X.size(); ++i) {
    m_localX.at(i).commitIf(t);
  }
}
