#include "invariants/linear.hpp"

#include <vector>

// TODO: invariant should take its true id in the constructor.
extern Id NULL_ID;

Linear::Linear(std::vector<Int> A, std::vector<VarId> X, VarId b)
    : Invariant(NULL_ID), m_A(A), m_X(X), m_b(b) {}


void Linear::init(Timestamp, Engine& e) {
  // precondition: this invariant must be registered with the engine before it
  // is initialised.
  assert(m_id != NULL_ID);

  e.registerDefinedVariable(m_b, m_id);
  for (size_t i = 0; i < m_X.size(); ++i) {
    LocalId li(i);
    e.registerInvariantDependsOnVar(m_id, m_X[i], li, m_A[i]);
  }
}

void Linear::recompute(Timestamp t, Engine& e) {
  Int sum = 0;
  for (size_t i = 0; i < m_X.size(); ++i) {
    sum += m_A[i] * e.getValue(t, m_X[i]);
  }
  // Dereference safe as incValue does not retain ptr.
  e.setValue(t, m_b, sum);
  // m_state.setValue(t, m_X.size());  // Not clear if we actually need to reset
  // this.
}

void Linear::notifyIntChanged(Timestamp t, Engine& e,
                              LocalId&, Int oldValue,
                              Int newValue, Int coef) {
  assert(newValue != oldValue);  // precondition
  e.incValue(t, m_b, (newValue - oldValue) * coef);
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
  Int delta = e.getValue(t, m_X.at(idx)) - e.getCommittedValue(m_X.at(idx));
  // assert(delta != 0);  // invariants are only notified when they are changed.
  // Invariants may now be notified even when they are not changed.
  // If an input is a view that did not change, but the source IntVar changed.
  if (delta == 0) {
    return;
  }
  e.incValue(t, m_b, delta * m_A.at(idx));
}

void Linear::commit(Timestamp t, Engine& e) {
  Invariant::commit(t,e);
}