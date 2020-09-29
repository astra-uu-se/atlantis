#include "invariants/elementVar.hpp"

#include <vector>

#include "core/engine.hpp"

// TODO: invariant should take its true id in the constructor.
extern Id NULL_ID;

ElementVar::ElementVar(VarId i, std::vector<VarId> X, VarId b)
    : Invariant(NULL_ID), m_i(i), m_X(X), m_b(b) {
  assert(b.idType == VarIdType::var);
}

void ElementVar::init([[maybe_unused]] Timestamp t, Engine& e) {
  assert(m_id != NULL_ID);

  LocalId localId(-1);

  e.registerDefinedVariable(m_b, m_id);
  e.registerInvariantDependsOnVar(m_id, m_i, LocalId(-1));
  for (size_t i = 0; i < m_X.size(); ++i) {
    e.registerInvariantDependsOnVar(m_id, m_X[i], LocalId(i));
  }
}

void ElementVar::recompute(Timestamp t, Engine& e) {
  e.setValue(t, m_b, e.getValue(t, m_X.at(e.getValue(t, m_i))));
}

void ElementVar::notifyIntChanged(Timestamp t, Engine& e, LocalId id,
                                  Int newValue) {
  if (id.id == LocalId(-1).id) {
    e.setValue(t, m_b, e.getValue(t, m_X.at(newValue)));
  } else if (id.id == static_cast<IdBase>(e.getValue(t, m_i))) {
    e.setValue(t, m_b, newValue);
  }
  // e.setValue(t, m_b, m_A.at(newValue));
}

VarId ElementVar::getNextDependency(Timestamp t, Engine& e) {
  m_state.incValue(t, 1);
  if (m_state.getValue(t) == 0) {
    return m_i;
  } else if (m_state.getValue(t) == 1) {
    return m_X.at(e.getValue(t, m_i));
  } else {
    return NULL_ID;  // Done
  }
}

void ElementVar::notifyCurrentDependencyChanged(Timestamp t, Engine& e) {
  assert(m_state.getValue(t) == 0 || m_state.getValue(t) == 1);
  e.setValue(t, m_b, e.getValue(t, m_X.at(e.getValue(t, m_i))));
}

void ElementVar::commit(Timestamp t, Engine& e) { Invariant::commit(t, e); }