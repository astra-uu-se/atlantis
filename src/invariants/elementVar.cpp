#include "invariants/elementVar.hpp"

// TODO: invariant should take its true id in the constructor.

ElementVar::ElementVar(VarId i, std::vector<VarId> X, VarId b)
    : Invariant(NULL_ID), m_i(i), m_X(std::move(X)), m_b(b) {}

void ElementVar::init([[maybe_unused]] Timestamp t, Engine& e) {
  assert(m_id != NULL_ID);

  e.registerDefinedVariable(m_b, m_id);
  e.registerInvariantDependsOnVar(m_id, m_i, LocalId(m_X.size()));
  for (size_t i = 0; i < m_X.size(); ++i) {
    e.registerInvariantDependsOnVar(m_id, m_X[i], LocalId(i));
  }
}

void ElementVar::compute(Timestamp t, Engine& e) {
  e.updateValue(
      t, m_b,
      e.getValue(t, m_X.at(static_cast<unsigned long>(e.getValue(t, m_i)))));
}

void ElementVar::recompute(Timestamp t, Engine& e) {
  e.updateValue(
      t, m_b,
      e.getValue(t, m_X.at(static_cast<unsigned long>(e.getValue(t, m_i)))));
  m_modifiedVariables.assign(m_X.size(), false);
}

void ElementVar::notifyIntChanged(Timestamp t, Engine& e, LocalId id) {
  if (id.id == LocalId(m_X.size()).id) {
    e.notifyMaybeChanged(t, m_b);
  } else if (id.id == static_cast<IdBase>(e.getValue(t, m_i))) {
    e.notifyMaybeChanged(t, m_b);
  }
  // e.setValue(t, m_b, m_A.at(newValue));
}

VarId ElementVar::getNextDependency(Timestamp t, Engine& e) {
  m_state.incValue(t, 1);
  if (m_state.getValue(t) == 0) {
    return m_i;
  } else if (m_state.getValue(t) == 1) {
    return m_X.at(static_cast<unsigned long>(e.getValue(t, m_i)));
  } else {
    return NULL_ID;  // Done
  }
}

void ElementVar::notifyCurrentDependencyChanged(Timestamp t, Engine& e) {
  assert(m_state.getValue(t) == 0 || m_state.getValue(t) == 1);
  e.updateValue(
      t, m_b,
      e.getValue(t, m_X.at(static_cast<unsigned long>(e.getValue(t, m_i)))));
}

void ElementVar::commit(Timestamp t, Engine& e) { Invariant::commit(t, e); }
