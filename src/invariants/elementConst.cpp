#include "invariants/elementConst.hpp"

// TODO: invariant should take its true id in the constructor.

ElementConst::ElementConst(VarId i, std::vector<Int> A, VarId b)
    : Invariant(NULL_ID), m_i(i), m_A(std::move(A)), m_b(b) {
  //  m_modifiedVars.resize(1, false);
  m_modifiedVars.reserve(1);
}

void ElementConst::init([[maybe_unused]] Timestamp t, Engine& e) {
  assert(m_id != NULL_ID);

  registerDefinedVariable(e, m_b);
  e.registerInvariantDependsOnVar(m_id, m_i, 0);
}

void ElementConst::recompute(Timestamp t, Engine& e) {
  updateValue(t, e, m_b,
              m_A.at(static_cast<unsigned long>(e.getValue(t, m_i))));
}

void ElementConst::notifyIntChanged(Timestamp t, Engine& e, LocalId) {
  auto newValue = e.getValue(t, m_i);
  updateValue(t, e, m_b, m_A.at(static_cast<unsigned long>(newValue)));
}

VarId ElementConst::getNextDependency(Timestamp t, Engine&) {
  m_state.incValue(t, 1);
  if (m_state.getValue(t) == 0) {
    return m_i;
  } else {
    return NULL_ID;  // Done
  }
}

void ElementConst::notifyCurrentDependencyChanged(Timestamp t, Engine& e) {
  assert(m_state.getValue(t) == 0);
  updateValue(t, e, m_b,
              m_A.at(static_cast<unsigned long>(e.getValue(t, m_i))));
}

void ElementConst::commit(Timestamp t, Engine& e) { Invariant::commit(t, e); }
