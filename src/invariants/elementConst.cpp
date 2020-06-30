#include "invariants/elementConst.hpp"

#include <vector>

// TODO: invariant should take its true id in the constructor.
extern Id NULL_ID;

ElementConst::ElementConst(VarId i, std::vector<Int> A, VarId b)
    : Invariant(NULL_ID), m_i(i), m_A(A), m_b(b) {}

void ElementConst::init([[maybe_unused]] Timestamp t, Engine& e) {
  assert(m_id != NULL_ID);

  e.registerDefinedVariable(m_b, m_id);
  e.registerInvariantDependsOnVar(m_id, m_i, 0, 0);
}

void ElementConst::recompute(Timestamp t, Engine& e) {
  e.setValue(t, m_b, m_A.at(e.getValue(t, m_i)));
}

void ElementConst::notifyIntChanged(Timestamp t, Engine& e, LocalId,
                                    Int oldValue, Int newValue, Int) {
  assert(newValue != oldValue);
  e.setValue(t, m_b, m_A.at(newValue));
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
  e.setValue(t, m_b, m_A.at(e.getValue(t, m_i)));
}

void ElementConst::commit(Timestamp t, Engine& e) {
  // todo: do nodes validate themself or is it done by engine?
  // this->validate(t);
  e.commitIf(t, m_b);
}