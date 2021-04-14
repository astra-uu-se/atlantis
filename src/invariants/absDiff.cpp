#include "invariants/absDiff.hpp"

extern Id NULL_ID;

AbsDiff::AbsDiff(VarId a, VarId b, VarId c)
    : Invariant(NULL_ID), m_a(a), m_b(b), m_c(c) {
  m_modifiedVars.reserve(1);
}

void AbsDiff::init([[maybe_unused]] Timestamp t, Engine& e) {
  assert(!m_id.equals(NULL_ID));

  registerDefinedVariable(e, m_c);
  e.registerInvariantDependsOnVar(m_id, m_a, 0);
  e.registerInvariantDependsOnVar(m_id, m_b, 0);
}

void AbsDiff::recompute(Timestamp t, Engine& e) {
  updateValue(t, e, m_c, std::abs(e.getValue(t, m_a) - e.getValue(t, m_b)));
}

void AbsDiff::notifyIntChanged(Timestamp t, Engine& e, LocalId) {
  notifyCurrentDependencyChanged(t, e);
}

VarId AbsDiff::getNextDependency(Timestamp t, Engine&) {
  m_state.incValue(t, 1);
  switch (m_state.getValue(t)) {
    case 0:
      return m_a;
    case 1:
      return m_b;
    default:
      return NULL_ID;
  }
}

void AbsDiff::notifyCurrentDependencyChanged(Timestamp t, Engine& e) {
  updateValue(t, e, m_c, std::abs(e.getValue(t, m_a) - e.getValue(t, m_b)));
}

void AbsDiff::commit(Timestamp t, Engine& e) { Invariant::commit(t, e); }
