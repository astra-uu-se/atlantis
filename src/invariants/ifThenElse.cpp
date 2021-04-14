#include "invariants/ifThenElse.hpp"

IfThenElse::IfThenElse(VarId b, VarId x, VarId y, VarId z)
    : Invariant(NULL_ID), m_b(b), m_xy({x, y}), m_z(z) {
  m_modifiedVars.reserve(1);
}

void IfThenElse::init([[maybe_unused]] Timestamp t, Engine& e) {
  assert(!m_id.equals(NULL_ID));

  registerDefinedVariable(e, m_z);
  e.registerInvariantDependsOnVar(m_id, m_b, 0);
  e.registerInvariantDependsOnVar(m_id, m_xy[0], 0);
  e.registerInvariantDependsOnVar(m_id, m_xy[1], 0);
}

void IfThenElse::recompute(Timestamp t, Engine& e) {
  auto b = e.getValue(t, m_b);
  updateValue(t, e, m_z, e.getValue(t, m_xy[1 - (b == 0)]));
}

void IfThenElse::notifyIntChanged(Timestamp t, Engine& e, LocalId) {
  notifyCurrentDependencyChanged(t, e);
}

VarId IfThenElse::getNextDependency(Timestamp t, Engine& e) {
  m_state.incValue(t, 1);
  auto state = m_state.getValue(t);
  if (state == 0) {
    return m_b;
  } else if (state == 1) {
    auto b = e.getValue(t, m_b);
    return m_xy[1 - (b == 0)];
  } else {
    return NULL_ID;  // Done
  }
}

void IfThenElse::notifyCurrentDependencyChanged(Timestamp t, Engine& e) {
  auto newValue = e.getValue(t, m_b);
  updateValue(t, e, m_z, e.getValue(t, m_xy[1 - (newValue == 0)]));
}

void IfThenElse::commit(Timestamp t, Engine& e) { Invariant::commit(t, e); }